#include <display_helpers.h>
#include <algorithm>

int Ticker::tick() {
    return ++ticks;
}

void Ticker::reset() {
    ticks = 0;
}

ScrollingWindow::ScrollingWindow(const int pTicksToWaitLong, const int pTicksToWaitShort) {
    ticksToWaitLong = pTicksToWaitLong;
    ticksToWaitShort = pTicksToWaitShort;
    skip = 0;
}

Window determineWindow(const int itemCount, const int suggestedStart, const int maxLen) {
    if (itemCount < maxLen) {
        return { 0, itemCount, itemCount };
    }
    const auto maxStart = itemCount - maxLen;
    const auto start = std::min(suggestedStart, maxStart);
    return  { start, start + maxLen, maxLen };
}

Window ScrollingWindow::getWindow(const int itemCount, const int maxLen) {
    const auto window = determineWindow(itemCount, skip, maxLen);

    const int ticksToWait = skip == 0 || itemCount == window.end ? ticksToWaitLong : ticksToWaitShort;
    if (ticker.tick() == ticksToWait) {
        if (itemCount == window.end) skip = 0; // continue from 0 next time
        else skip++;
        ticker.reset();
    }

    return window;
}

Line::Line(const std::string &pLeftText, const std::string &pRightText): scroller(10, 3) {
    leftText = pLeftText;
    rightText = pRightText;
}

bool Line::isValid() {
    // a line is valid for max 600 ticks
    return ticker.tick() < 600;
}

void Line::update(const std::string &pRightText) {
    rightText = pRightText;
    ticker.reset();
}

std::string Line::getKey() const {
    return leftText;
}

std::string Line::get(const int width) {
    const auto elipses = "..";
    const auto leftLen = static_cast<int>(leftText.length());
    const auto rightLen = static_cast<int>(rightText.length());
    const auto maxLeftLen = std::max(0, width - rightLen - 1); // count one character for space between left and right

    const auto window = scroller.getWindow(leftLen, maxLeftLen);

    const std::string addition = leftLen > window.end ? elipses : "";
    const auto text = leftText.substr(window.start, window.cnt - addition.length());

    const std::string fillText(width - window.cnt - rightLen, ' ');

    return text + addition + fillText + rightText;
}

DisplayBuffer::DisplayBuffer(): scroller(40, 20) {
}

Line* DisplayBuffer::find(const std::string key) {
    for(auto &line: lines) {
        if (line.getKey() == key) {
            return &line;
        }
    }
    return nullptr;
}

void DisplayBuffer::addLine(const std::string &leftText, const std::string &rightText) {
    auto currentLine = find(leftText);
    if (currentLine) {
        currentLine->update(rightText);
    } else {
        lines.push_back(Line(leftText, rightText));
    }
}

void DisplayBuffer::purge() {
    std::vector<Line> newLines;
    for(auto &line: lines) {
        if (line.isValid()) {
            newLines.push_back(line);
        }
    }
    lines.clear();
    lines = newLines;
}

std::vector<std::string> DisplayBuffer::getTextToDisplay(const int width, const int height) {
    purge();

    const auto lineCnt = lines.size();
    const auto window = scroller.getWindow(lineCnt, height - 1);

    std::vector<std::string> toDisplay;
    for(int i=window.start; i<window.end; i++) {
        toDisplay.push_back(lines.at(i).get(width));
    }
    if (window.cnt < lineCnt) {
        Line line("", std::string(".. +") + std::to_string(lineCnt - window.cnt) + " lines");
        toDisplay.push_back(line.get(width));
    }
    return toDisplay;
}
