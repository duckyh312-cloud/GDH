#pragma once

#include <Geode/Geode.hpp>
#include <array>
#include <vector>
#include <unordered_map>
#include <string>
#include <queue>

namespace GDH {
    namespace Labels {
        enum Corner {
            Top_Left,
            Top_Center,
            Top_Right,
            Center_Left,
            Center_Center,
            Center_Right,
            Bottom_Left,
            Bottom_Center,
            Bottom_Right,
        };

        enum LabelType {
            Text,
            Spacing,
        };

        class Label {
        public:
            bool enabled;
            LabelType type;
            std::string text;
            std::array<float, 4> color;
            float size;
            
            bool rainbow;
            bool cps;
            bool noclip;

            Label(std::string text, std::array<float, 4> color, float size = 0.3f, bool rainbow = false, bool cps = false, bool noclip = false) {
                this->enabled = true;
                this->type = LabelType::Text;
                this->text = text;
                this->color = color;
                this->size = size;
                this->rainbow = rainbow;
                this->cps = cps;
                this->noclip = noclip;
            }
            
            Label(float spacing) {
                this->enabled = true;
                this->type = LabelType::Spacing;
                this->text = "";
                this->color = {1.0f, 1.0f, 1.0f, 1.0f};
                this->size = spacing;
                this->rainbow = false;
                this->cps = false;
                this->noclip = false;
            }

            std::string render() const;
        };

        class Manager {
        public:
            static Manager& get() {
                static Manager instance;
                return instance;
            }

            Manager& operator=(const Manager&) = delete;
            Manager(const Manager&) = delete;

            float cornerPadding = 4.0f;
            float midPadding = 2.0f;
            std::unordered_map<Corner, std::vector<Label>> labels;

            void save();
            void load();

        private:
            Manager() = default;
        };
    } // namespace Labels
} // namespace GDH

class NoclipAccuracy {
public:    
    static NoclipAccuracy& get() {
        static NoclipAccuracy instance;
        return instance;
    }

    NoclipAccuracy& operator=(const NoclipAccuracy&) = delete;
    NoclipAccuracy(const NoclipAccuracy&) = delete;

    int frames = 0, deaths = 0, deaths_full = 0;
    bool wouldDie = false, prevDied = false;

    void handle_update(GJBaseGameLayer* self, float delta) {
        auto pl = PlayLayer::get();
        if (!pl || self->m_player1->m_isDead || pl->m_levelEndAnimationStarted) {
            prevDied = false;
            return;
        }

        frames++;

        if (wouldDie) {
            wouldDie = false;
            deaths++;

            if (!prevDied) {
                deaths_full++;
                prevDied = true;
            }
        } else {
            prevDied = false;
        }
    }

    void handle_reset() {
        frames = 0;
        deaths = 0;
        deaths_full = 0;
        wouldDie = false;
        prevDied = false;
    }

    void handle_death() {
        wouldDie = true;
    }

    float getPercentage() const {
        if (frames == 0) return 100.0f;
        return (1.0f - static_cast<float>(deaths) / frames) * 100.0f;
    }

private:
    NoclipAccuracy() = default;
};

class CpsCounter {
public:
    static CpsCounter& get() {
        static CpsCounter instance;
        return instance;
    }

    CpsCounter(const CpsCounter&) = delete;
    CpsCounter& operator=(const CpsCounter&) = delete;

    std::queue<std::chrono::steady_clock::time_point> clicks;

    int cps = 0;
    int highscore = 0;
    int overall = 0;
    
    void reset() {
        cps = 0;
        highscore = 0;
        overall = 0;
        std::queue<std::chrono::steady_clock::time_point>().swap(clicks);
    }
    
    void click() {
        overall++;
        clicks.push(std::chrono::steady_clock::now());
    }
    
    void update() {
        auto one_second_ago = std::chrono::steady_clock::now() - std::chrono::seconds(1);
        
        while (!clicks.empty() && clicks.front() < one_second_ago) {
            clicks.pop();
        }

        cps = static_cast<int>(clicks.size());

        if (highscore < cps) {
            highscore = cps;
        }
    }
private:
    CpsCounter() = default;
};