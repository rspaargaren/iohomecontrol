#include <string>
#include <vector>

class Ticker {
    private:
        int ticks = 0;
    public:
        int tick();
        void reset();
};

struct Window {
    const int start;
    const int end;
    const int cnt;
};

class ScrollingWindow {
    private: 
        int skip;
        int ticksToWaitLong;
        int ticksToWaitShort;
        Ticker ticker;
    public:
        ScrollingWindow(const int pTicksToWaitLong, const int pTicksToWaitShort);
        Window getWindow(const int itemCount, const int maxLen);
};

class Line {
    private:
        std::string leftText;
        std::string rightText;
        Ticker ticker;
        ScrollingWindow scroller;  
    public:
        Line(const std::string &pLeftText, const std::string &pRightText);

        bool isValid();

        void update(const std::string &pRightText);

        std::string getKey() const;

        std::string get(const int width);
};
 
class DisplayBuffer {
    private:
        std::vector<Line> lines;
        ScrollingWindow scroller;

        Line* find(const std::string key);
    public:
        DisplayBuffer();
    
        void addLine(const std::string &leftText, const std::string &rightText = "");

        void purge();

        std::vector<std::string> getTextToDisplay(const int width, const int height);
};