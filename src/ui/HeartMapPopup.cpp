#include "HeartMapPopup.hpp"
#include "HeartMap.hpp"
#include "HeartStyle.hpp"
#include "HudEditor.hpp"
#include "LevelStats.hpp"
#include "Zones.hpp"

#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/NineSlice.hpp>

using namespace geode::prelude;

namespace {
    // Kept below ~290 so the close button isn't cut off on 16:9 screens.
    constexpr CCSize kPopupSize = { 420.f, 280.f };
    constexpr CCSize kGraphSize = { 380.f, 108.f };
    constexpr CCSize kCardSize = { 122.f, 46.f };

    // Space inside the graph panel for the BPM labels (left) and heat band (bottom).
    constexpr float kPadLeft = 32.f;
    constexpr float kPadRight = 12.f;
    constexpr float kPadTop = 12.f;
    constexpr float kPlotBottom = 26.f;
    constexpr float kBandBottom = 9.f;
    constexpr float kBandHeight = 8.f;
    constexpr float kLineRadius = 1.4f;
    // Buckets further apart than this are not connected by a line.
    constexpr int kMaxGap = 4;

    constexpr ccColor3B kLabelColor = { 235, 215, 185 };
    constexpr ccColor3B kDetailColor = { 215, 190, 160 };
    constexpr ccColor4F kGold = { 1.f, 0.78f, 0.1f, 1.f };
    constexpr ccColor4F kGoldDark = { 0.35f, 0.18f, 0.f, 1.f };
    constexpr ccColor4F kOutline = { 0.1f, 0.05f, 0.02f, 1.f };

    CCDrawNode* createDrawNode() {
        auto draw = CCDrawNode::create();
        // Our colors are not premultiplied, so blend with the normal alpha.
        draw->setBlendFunc({ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA });
        return draw;
    }

    // Dark rounded panel with a lighter outline, like an inset box.
    // Children use the panel's local coordinates (0,0 = bottom left).
    CCNode* createPanel(CCSize size) {
        constexpr float kBorder = 1.5f;

        auto panel = CCNode::create();
        panel->setContentSize(size);
        panel->setAnchorPoint({ .5f, .5f });

        // square02b is the white version of GD's rounded box, so it can be tinted.
        auto outline = NineSlice::create("square02b_001.png");
        outline->setColor({ 196, 112, 52 });
        outline->setContentSize(size + CCSize{ kBorder * 2, kBorder * 2 });
        outline->setPosition(size / 2);
        panel->addChild(outline);

        auto fill = NineSlice::create("square02b_001.png");
        fill->setColor({ 74, 37, 16 });
        fill->setContentSize(size);
        fill->setPosition(size / 2);
        panel->addChild(fill);

        return panel;
    }

    void drawDashedLine(CCDrawNode* draw, CCPoint from, CCPoint to, ccColor4F color) {
        constexpr float kDash = 4.f;
        constexpr float kGap = 3.f;
        auto delta = to - from;
        float length = delta.getLength();
        auto dir = delta / length;
        for (float pos = 0.f; pos < length; pos += kDash + kGap) {
            float end = std::min(pos + kDash, length);
            draw->drawSegment(from + dir * pos, from + dir * end, 0.35f, color);
        }
    }

    // Round marker with a dark outline.
    void drawMarker(CCDrawNode* draw, CCPoint pos, float radius, ccColor4F color) {
        draw->drawCircle(pos, radius, color, 1.2f, kOutline, 24);
    }

    // Three rising bars, used for the average card.
    CCNode* createBarsIcon() {
        auto draw = createDrawNode();
        float heights[] = { 8.f, 13.f, 19.f };
        for (int i = 0; i < 3; ++i) {
            float x = i * 7.f;
            draw->drawRect(CCPoint{ x, 0.f }, CCPoint{ x + 5.f, heights[i] }, kGold, 0.8f, kGoldDark);
        }
        draw->setContentSize({ 19.f, 19.f });
        draw->setAnchorPoint({ .5f, .5f });
        return draw;
    }

    // Golden block stairs for the bottom corners. Drawn from rectangles so the
    // edges stay crisp; `right` mirrors them for the right corner.
    CCNode* createStairs(bool right) {
        constexpr float kBlock = 9.f;
        constexpr ccColor4F kBlockFill = { 1.f, 0.76f, 0.12f, 1.f };
        constexpr ccColor4F kBlockEdge = { 0.45f, 0.22f, 0.f, 1.f };
        constexpr ccColor4F kHighlight = { 1.f, 0.92f, 0.5f, 1.f };
        constexpr int kHeights[] = { 4, 3, 2, 1 };

        auto draw = createDrawNode();
        float width = kBlock * std::size(kHeights);
        for (int column = 0; column < static_cast<int>(std::size(kHeights)); ++column) {
            for (int row = 0; row < kHeights[column]; ++row) {
                float x = right ? width - (column + 1) * kBlock : column * kBlock;
                float y = row * kBlock;
                draw->drawRect(CCPoint{ x, y }, CCPoint{ x + kBlock, y + kBlock }, kBlockFill, 0.7f, kBlockEdge);
                // Light strip on top of each block
                draw->drawRect(CCPoint{ x + 1.5f, y + kBlock - 2.5f }, CCPoint{ x + kBlock - 1.5f, y + kBlock - 1.5f },
                    kHighlight, 0.f, kHighlight);
            }
        }
        draw->setContentSize({ width, kBlock * kHeights[0] });
        return draw;
    }

    CCLabelBMFont* createLabel(char const* text, char const* font, float scale, ccColor3B color = ccWHITE) {
        auto label = CCLabelBMFont::create(text, font);
        label->setScale(scale);
        label->setColor(color);
        return label;
    }
}

HeartGraph* HeartGraph::create(CCSize size, HeartTrack const& attempt, HeartTrack const& heat) {
    auto ret = new HeartGraph();
    if (ret->init(size, attempt, heat)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool HeartGraph::init(CCSize size, HeartTrack const& attempt, HeartTrack const& heat) {
    if (!CCNode::init()) return false;

    this->setContentSize(size);
    this->setAnchorPoint({ .5f, .5f });

    auto panel = createPanel(size);
    panel->setPosition(size / 2);
    this->addChild(panel);

    auto draw = createDrawNode();
    this->addChild(draw);

    float plotLeft = kPadLeft;
    float plotRight = size.width - kPadRight;
    float plotBottom = kPlotBottom;
    float plotTop = size.height - kPadTop;
    float bucketWidth = (plotRight - plotLeft) / HeartTrack::kBuckets;

    // BPM range: rounded to 10s around the data, at least 20 BPM tall.
    float low = 1000.f, high = 0.f;
    for (auto const* track : { &attempt, &heat }) {
        for (int i = 0; i < HeartTrack::kBuckets; ++i) {
            if (!track->hasBucket(i)) continue;
            low = std::min(low, track->bucketAverage(i));
            high = std::max(high, track->bucketAverage(i));
        }
    }
    high = std::max(high, static_cast<float>(attempt.peakBpm));
    low = std::floor((low - 5.f) / 10.f) * 10.f;
    high = std::ceil((high + 5.f) / 10.f) * 10.f;
    if (high - low < 20.f) high = low + 20.f;

    auto toX = [&](float percent) { return plotLeft + (plotRight - plotLeft) * percent / 100.f; };
    auto toY = [&](float bpm) { return plotBottom + (plotTop - plotBottom) * (bpm - low) / (high - low); };
    auto bucketCenter = [](int i) { return (i + .5f) * 100.f / HeartTrack::kBuckets; };

    // Dashed grid: BPM lines with labels, and every 20% of the level.
    ccColor4F grid = { 1.f, 0.9f, 0.8f, 0.18f };
    for (float bpm : { low, (low + high) / 2.f, high }) {
        float y = toY(bpm);
        drawDashedLine(draw, { plotLeft, y }, { plotRight, y }, grid);

        auto label = createLabel(fmt::format("{:.0f}", bpm).c_str(), "bigFont.fnt", 0.3f, kLabelColor);
        label->setAnchorPoint({ 1.f, .5f });
        label->setPosition({ plotLeft - 5.f, y });
        this->addChild(label);
    }
    for (int percent = 20; percent < 100; percent += 20) {
        float x = toX(static_cast<float>(percent));
        drawDashedLine(draw, { x, plotBottom }, { x, plotTop }, grid);
    }

    // Axes
    ccColor4F axis = { 1.f, 0.9f, 0.8f, 0.45f };
    draw->drawSegment({ plotLeft, plotBottom }, { plotLeft, plotTop }, 0.5f, axis);
    draw->drawSegment({ plotLeft, plotBottom }, { plotRight, plotBottom }, 0.5f, axis);

    // All-time heat band along the bottom.
    for (int i = 0; i < HeartTrack::kBuckets; ++i) {
        if (!heat.hasBucket(i)) continue;
        auto color = zones::toColor4F(zones::color(heat.bucketAverage(i)));
        float x = plotLeft + i * bucketWidth;
        draw->drawRect(CCPoint{ x, kBandBottom }, CCPoint{ x + bucketWidth, kBandBottom + kBandHeight }, color, 0.f, color);
    }

    // Latest attempt as a line, colored by zone. Runs of the same value are
    // merged into one segment - fewer, longer segments draw much cleaner.
    std::vector<std::vector<CCPoint>> runs;
    int previous = -1;
    for (int i = 0; i < HeartTrack::kBuckets; ++i) {
        if (!attempt.hasBucket(i)) continue;
        CCPoint point = { toX(bucketCenter(i)), toY(attempt.bucketAverage(i)) };
        if (previous < 0 || i - previous > kMaxGap) {
            runs.push_back({ point });
        }
        else {
            auto& run = runs.back();
            bool flat = run.size() >= 2
                && std::abs(run[run.size() - 1].y - point.y) < 0.05f
                && std::abs(run[run.size() - 2].y - point.y) < 0.05f;
            if (flat) run.back() = point;
            else run.push_back(point);
        }
        previous = i;
    }
    auto toBpm = [&](float y) { return low + (y - plotBottom) / (plotTop - plotBottom) * (high - low); };
    for (auto const& run : runs) {
        for (size_t i = 0; i + 1 < run.size(); ++i) {
            float bpm = toBpm((run[i].y + run[i + 1].y) / 2.f);
            draw->drawSegment(run[i], run[i + 1], kLineRadius, zones::toColor4F(zones::color(bpm)));
        }
    }

    if (!attempt.empty()) {
        auto bucketOf = [](float percent) {
            return std::min(HeartTrack::kBuckets - 1, static_cast<int>(percent / 100.f * HeartTrack::kBuckets));
        };

        // Where the attempt ended (death or finish)
        int endBucket = bucketOf(attempt.endPercent);
        if (attempt.hasBucket(endBucket)) {
            drawMarker(draw, { toX(attempt.endPercent), toY(attempt.bucketAverage(endBucket)) }, 3.f,
                zones::toColor4F(zones::kPanic));
        }

        // Peak marker, drawn last so it stays on top
        CCPoint peak = { toX(attempt.peakPercent), toY(attempt.bucketAverage(bucketOf(attempt.peakPercent))) };
        drawMarker(draw, peak, 3.f, zones::toColor4F(ccWHITE));

        auto peakLabel = createLabel(fmt::format("{}", attempt.peakBpm).c_str(), "bigFont.fnt", 0.35f);
        // Near the left edge, move the label right so it doesn't cover the
        // BPM labels of the axis.
        float minX = plotLeft + peakLabel->getScaledContentWidth() / 2 + 2.f;
        peakLabel->setPosition({ std::max(peak.x, minX), peak.y + 10.f });
        this->addChild(peakLabel);
    }

    return true;
}

HeartMapPopup* HeartMapPopup::create(bool showHudButton) {
    auto ret = new HeartMapPopup();
    if (ret->initPopup(showHudButton)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool HeartMapPopup::initPopup(bool showHudButton) {
    if (!Popup::init(kPopupSize.width, kPopupSize.height)) return false;

    this->setID("heart-map-popup"_spr);
    this->setTitle("Heart Map", "goldFont.fnt", 0.9f, 24.f);
    this->addTitleDecoration();
    this->addCornerDecoration();

    auto const& map = HeartMap::get();
    auto const& attempt = map.latestAttempt();
    auto allTime = map.allTime();

    this->addSubtitle(attempt, &attempt == &map.currentAttempt());
    this->addBranding(showHudButton);

    float cardsY = 64.f;

    if (allTime.empty() && attempt.empty()) {
        auto empty = CCLabelBMFont::create(
            "No heart rate recorded yet.\nPlay with HypeRate connected\nto see your Heart Map.",
            "bigFont.fnt", 300.f, kCCTextAlignmentCenter
        );
        empty->setScale(0.45f);
        m_mainLayer->addChildAtPosition(empty, Anchor::Center, { 0.f, 10.f });
    }
    else {
        float graphY = kPopupSize.height - 56.f - kGraphSize.height / 2;
        auto graph = HeartGraph::create(kGraphSize, attempt, allTime);
        graph->setPosition({ kPopupSize.width / 2, graphY });
        m_mainLayer->addChild(graph);

        auto legend = this->createLegend();
        legend->setPosition({ kPopupSize.width / 2, graphY - kGraphSize.height / 2 - 11.f });
        m_mainLayer->addChild(legend);

        auto heart = CCSprite::create(heart_style::current());
        heart->setColor(zones::kPanic);
        heart->setScale(0.7f);
        this->addStatCard(
            heart, "Peak",
            attempt.empty() ? "-" : fmt::format("{} BPM", attempt.peakBpm),
            attempt.empty() ? "" : fmt::format("at {:.0f}%", attempt.peakPercent),
            { -(kCardSize.width + 8.f), cardsY }
        );

        this->addStatCard(
            createBarsIcon(), "Average",
            attempt.empty() ? "-" : fmt::format("{:.0f} BPM", attempt.average()),
            "this attempt",
            { 0.f, cardsY }
        );

        // All-time record of this level (saved attempts only).
        auto const& record = LevelStats::get().find(map.levelKey());
        CCNode* clock = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
        if (clock) clock->setScale(0.8f);
        std::string recordValue = record.completions > 0
            ? fmt::format("Clear {:.0f} BPM", record.calmestClear)
            : fmt::format("Best {:.0f}%", record.bestPercent);
        this->addStatCard(
            clock, "Record",
            record.attempts > 0 ? recordValue : "-",
            record.attempts > 0
                ? fmt::format("{} att. - peak {}", record.attempts, record.heat.peakBpm)
                : "no saved runs yet",
            { kCardSize.width + 8.f, cardsY }
        );
    }

    if (showHudButton) {
        auto hudButton = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("Edit HUD", "goldFont.fnt", "GJ_button_01.png", 0.9f),
            [this](auto) {
                // Open the editor first - closing may release this popup.
                HudEditor::create()->show();
                this->onClose(nullptr);
            }
        );
        hudButton->setScale(0.8f);
        hudButton->m_baseScale = 0.8f;
        hudButton->setID("edit-hud-button"_spr);
        m_buttonMenu->addChildAtPosition(hudButton, Anchor::Bottom, { 0.f, 24.f });
    }

    return true;
}

// "Stereo Madness  -  Level complete" under the title, so a screenshot of the
// popup explains itself.
void HeartMapPopup::addSubtitle(HeartTrack const& attempt, bool isCurrentAttempt) {
    auto playLayer = PlayLayer::get();
    if (!playLayer || !playLayer->m_level) return;

    std::string text = playLayer->m_level->m_levelName;
    if (!attempt.empty()) {
        bool alive = playLayer->m_player1 && !playLayer->m_player1->m_isDead;
        if (isCurrentAttempt && playLayer->m_hasCompletedLevel) {
            text += "  -  Level complete";
        }
        else if (isCurrentAttempt && alive) {
            text += fmt::format("  -  {:.0f}% so far", attempt.endPercent);
        }
        else {
            text += fmt::format("  -  Died at {:.0f}%", attempt.endPercent);
        }
    }

    auto label = createLabel(text.c_str(), "bigFont.fnt", 0.4f, kLabelColor);
    label->limitLabelWidth(kPopupSize.width - 80.f, 0.4f, 0.1f);
    label->setPosition({ kPopupSize.width / 2, kPopupSize.height - 42.f });
    m_mainLayer->addChild(label);
}

// Small HypeRate logo + "hyperate.io" at the bottom.
void HeartMapPopup::addBranding(bool nextToButton) {
    constexpr float kLogoSize = 14.f;

    auto branding = CCNode::create();
    branding->setAnchorPoint({ .5f, .5f });

    auto logo = createModLogo(Mod::get());
    limitNodeSize(logo, { kLogoSize, kLogoSize }, 1.f, 0.01f);
    logo->setAnchorPoint({ .5f, .5f });
    logo->setPosition({ kLogoSize / 2, 0.f });
    branding->addChild(logo);

    auto label = createLabel("hyperate.io", "bigFont.fnt", 0.3f);
    label->setAnchorPoint({ 0.f, .5f });
    label->setPosition({ kLogoSize + 4.f, 0.f });
    branding->addChild(label);

    branding->setContentSize({ kLogoSize + 4.f + label->getScaledContentWidth(), 0.f });
    // Right of the Edit HUD button when it is there, centered otherwise.
    branding->setPosition({ nextToButton ? kPopupSize.width / 2 + 108.f : kPopupSize.width / 2, 20.f });
    m_mainLayer->addChild(branding, 2);
}

void HeartMapPopup::addTitleDecoration() {
    if (!m_title) return;
    float offset = m_title->getScaledContentWidth() / 2 + 22.f;
    for (float side : { -1.f, 1.f }) {
        auto icon = CCSprite::create("pulse.png"_spr);
        icon->setPosition(m_title->getPosition() + CCPoint{ side * offset, 0.f });
        m_mainLayer->addChild(icon);
    }
}

void HeartMapPopup::addCornerDecoration() {
    constexpr float kInset = 5.f;
    for (bool right : { false, true }) {
        auto stairs = createStairs(right);
        stairs->setAnchorPoint({ right ? 1.f : 0.f, 0.f });
        stairs->setPosition({ right ? kPopupSize.width - kInset : kInset, kInset });
        m_mainLayer->addChild(stairs, 1);
    }
}

// [line] Latest attempt    [heat gradient] All-time heatmap
CCNode* HeartMapPopup::createLegend() {
    constexpr float kSwatchWidth = 22.f;
    constexpr float kGap = 6.f;
    constexpr float kItemGap = 34.f;

    auto legend = CCNode::create();
    legend->setAnchorPoint({ .5f, .5f });

    auto draw = createDrawNode();
    legend->addChild(draw);

    auto makeLabel = [&](char const* text) {
        auto label = createLabel(text, "bigFont.fnt", 0.32f);
        label->setAnchorPoint({ 0.f, .5f });
        legend->addChild(label);
        return label;
    };

    float x = 0.f;
    draw->drawSegment({ x + 2.f, 0.f }, { x + kSwatchWidth - 2.f, 0.f }, 2.f, zones::toColor4F(zones::kCalm));
    x += kSwatchWidth + kGap;
    auto attemptLabel = makeLabel("Latest attempt");
    attemptLabel->setPosition({ x, 0.f });
    x += attemptLabel->getScaledContentWidth() + kItemGap;

    // Heat gradient swatch with a dark border: calm -> tense -> panic
    constexpr int kSteps = 10;
    constexpr float kHalfHeight = 4.f;
    draw->drawRect(CCPoint{ x - 1.f, -kHalfHeight - 1.f }, CCPoint{ x + kSwatchWidth + 1.f, kHalfHeight + 1.f },
        kOutline, 0.f, kOutline);
    for (int i = 0; i < kSteps; ++i) {
        float t = i / static_cast<float>(kSteps - 1);
        auto color = t < .5f
            ? zones::lerp(zones::kCalm, zones::kTense, t * 2.f)
            : zones::lerp(zones::kTense, zones::kPanic, (t - .5f) * 2.f);
        auto color4 = zones::toColor4F(color);
        float stepWidth = kSwatchWidth / kSteps;
        draw->drawRect(CCPoint{ x + i * stepWidth, -kHalfHeight }, CCPoint{ x + (i + 1) * stepWidth, kHalfHeight },
            color4, 0.f, color4);
    }
    x += kSwatchWidth + kGap;
    auto heatLabel = makeLabel("All-time heatmap");
    heatLabel->setPosition({ x, 0.f });
    x += heatLabel->getScaledContentWidth();

    // Children are laid out around y = 0, the node is centered horizontally.
    legend->setContentSize({ x, 0.f });
    return legend;
}

void HeartMapPopup::addStatCard(
    CCNode* icon, char const* title, std::string const& value,
    std::string const& detail, CCPoint position
) {
    auto card = createPanel(kCardSize);
    // x is relative to the popup center, y to its bottom edge.
    card->setPosition({ kPopupSize.width / 2 + position.x, position.y });
    m_mainLayer->addChild(card);

    constexpr float kIconX = 20.f;
    float textX = kIconX + (kCardSize.width - kIconX) / 2 + 4.f;
    float textWidth = kCardSize.width - kIconX * 2 - 6.f;

    if (icon) {
        icon->setPosition({ kIconX, kCardSize.height / 2 });
        card->addChild(icon);
    }

    auto titleLabel = createLabel(title, "goldFont.fnt", 0.5f);
    titleLabel->setPosition({ textX, kCardSize.height - 10.f });
    card->addChild(titleLabel);

    auto valueLabel = createLabel(value.c_str(), "bigFont.fnt", 0.45f);
    valueLabel->limitLabelWidth(textWidth, 0.45f, 0.1f);
    valueLabel->setPosition({ textX, detail.empty() ? 17.f : 22.f });
    card->addChild(valueLabel);

    if (!detail.empty()) {
        auto detailLabel = createLabel(detail.c_str(), "bigFont.fnt", 0.28f, kDetailColor);
        detailLabel->limitLabelWidth(textWidth, 0.28f, 0.1f);
        detailLabel->setPosition({ textX, 9.f });
        card->addChild(detailLabel);
    }
}
