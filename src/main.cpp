#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

#define SETTING(type, key_name) Mod::get()->getSettingValue<type>(key_name)

class ModSettingsPopup : public Popup<Mod*> {};

$on_mod(Loaded) {
    //more search paths
    auto search_paths = {
        getMod()->getConfigDir().string(),
        getMod()->getSaveDir().string(),
        getMod()->getTempDir().string()
    };
    for (auto entry : search_paths) CCFileUtils::get()->addPriorityPath(
        entry.c_str()
    );

    //gif file preload
    auto BACKGROUND_FILE = string::pathToString(SETTING(std::filesystem::path, "BACKGROUND_FILE"));
    BACKGROUND_FILE = CCFileUtils::get()->fullPathForFilename(BACKGROUND_FILE.c_str(), 0).c_str();
    CCSprite::create(BACKGROUND_FILE.c_str());
}

#include <Geode/modify/MenuGameLayer.hpp>
class $modify(MenuGameLayerExt, MenuGameLayer) {
public:
    bool init() {

        auto loadedBgID = GameManager::get()->m_loadedBgID;
        auto loadedGroundID = GameManager::get()->m_loadedGroundID;

        if (auto id = SETTING(int, "FORCE_BACKGROUND_ID")) GameManager::get()->m_loadedBgID = id;
        if (auto id = SETTING(int, "FORCE_GROUND_ID")) GameManager::get()->m_loadedGroundID = id;

        if (!MenuGameLayer::init()) return false;

        GameManager::get()->m_loadedBgID = loadedBgID;
        GameManager::get()->m_loadedGroundID = loadedGroundID;

        auto BACKGROUND_FILE = string::pathToString(SETTING(std::filesystem::path, "BACKGROUND_FILE"));
        BACKGROUND_FILE = CCFileUtils::get()->fullPathForFilename(BACKGROUND_FILE.c_str(), 0).c_str();

        CCNodeRGBA* inital_sprite = CCSprite::create(BACKGROUND_FILE.c_str());

        if (inital_sprite) {

            auto size = this->getContentSize();

            auto scroll = ScrollLayer::create(size);
            scroll->setID("background-layer"_spr);

            scroll->m_contentLayer->setPositionX(SETTING(float, "BG_POSX"));
            scroll->m_contentLayer->setPositionY(SETTING(float, "BG_POSY"));

            auto setupSprite = [scroll, size](CCNode* sprite = nullptr)
                {
                    scroll->m_contentLayer->removeChildByID("sprite"_spr);
                    auto BACKGROUND_FILE = string::pathToString(SETTING(std::filesystem::path, "BACKGROUND_FILE"));
                    BACKGROUND_FILE = CCFileUtils::get()->fullPathForFilename(BACKGROUND_FILE.c_str(), 0).c_str();
                    if (!sprite) sprite = CCSprite::create(BACKGROUND_FILE.c_str());
                    sprite = sprite ? sprite : CCLabelBMFont::create("FAILED TO LOAD IMAGE", "bigFont.fnt");
                    if (sprite) {
                        sprite->setPosition({ size.width * 0.5f, size.height * 0.5f });
                        sprite->setID("sprite"_spr);
                        sprite->runAction(CCRepeatForever::create(CCSpawn::create(CallFuncExt::create([sprite] {sprite->setVisible(1); }), nullptr)));
                        scroll->m_contentLayer->addChild(sprite);
                    };
                };

            setupSprite(inital_sprite);

            scroll->m_disableHorizontal = 0;
            scroll->m_scrollLimitTop = 9999;
            scroll->m_scrollLimitBottom = 9999;
            scroll->m_cutContent = false;

            scroll->setMouseEnabled(SETTING(bool, "SETUP_MODE_ENABLED"));

            auto scroll_bg = CCSprite::create("groundSquare_18_001.png");//geode::createLayerBG();
            scroll_bg->setScale(9999.f);
            scroll_bg->setColor(ccBLACK);
            scroll->setID("bg"_spr);
            scroll->addChild(scroll_bg, -1);

            scroll->runAction(CCRepeatForever::create(CCSpawn::create(CallFuncExt::create(
                [scroll, scroll_bg, setupSprite, this] {
                    auto content = scroll->m_contentLayer;

                    scroll->m_disableMovement = not SETTING(bool, "SETUP_MODE_ENABLED");
                    scroll->setMouseEnabled(SETTING(bool, "SETUP_MODE_ENABLED"));

                    if (auto a = scroll->getChildByIDRecursive("menu_top_container"_spr)) a->setScale(SETTING(float, "SETUP_WINDOW_SCALE"));

                    if (auto a = this->getChildByType<GJGroundLayer>(-1)) a->setVisible(SETTING(bool, "HIDE_GROUND"));

                    scroll_bg->setVisible(SETTING(bool, "HIDE_GAME"));
                    scroll->setZOrder(SETTING(bool, "HIDE_GAME") ? 999 : SETTING(int, "BG_ZORDER"));

                    if (auto a = SETTING(float, "BG_SCALEX")) content->setScaleX(a); else content->setScaleX(1.f);
                    if (auto a = SETTING(float, "BG_SCALEY")) content->setScaleY(a); else content->setScaleY(1.f);
                    if (!(bool)SETTING(float, "BG_SCALEX") and !(bool)SETTING(float, "BG_SCALEY")) {
                        auto sprite = content->getChildByID("sprite"_spr); 
                        sprite->setScale(1.f);
                        //"one-of": ["Fullscreen Stretch", "Up to WinHeight", "Up to WinWidth", "Up to WinSize", "NONE"]
                        auto type = SETTING(std::string, "BG_SCALE_TYPE");
                        if (type == "Fullscreen Stretch") {
                            sprite->setScaleX((content->getContentWidth() / sprite->getContentWidth()));
                            sprite->setScaleY((content->getContentHeight() / sprite->getContentHeight()));
                        }
                        if (type == "Up to WinHeight") cocos::limitNodeHeight(sprite, content->getContentHeight(), 9999.f, 0.1f);
                        if (type == "Up to WinWidth") cocos::limitNodeWidth(sprite, content->getContentWidth(), 9999.f, 0.1f);
                        if (type == "Up to WinSize") cocos::limitNodeSize(sprite, content->getContentSize(), 9999.f, 0.1f);
                    }

                    if (not SETTING(bool, "SETUP_MODE_ENABLED")) {

                        content->setPositionX(SETTING(float, "BG_POSX"));
                        content->setPositionY(SETTING(float, "BG_POSY"));

                    }
                    else {

                        Mod::get()->setSettingValue("BG_POSX", content->getPositionX());
                        Mod::get()->setSettingValue("BG_POSY", content->getPositionY());

                        if (auto sc = CCScene::get()) if (sc->getChildByType<CCLayer>(0)) {
                            static Ref<Popup<Mod*>> popup;
                            if (!sc->getChildByType<ModSettingsPopup>(0)) popup = openSettingsPopup(
                                getMod(), true //openSettingsPopup
                            );
                            else if (popup and popup->isRunning()) {
                                popup->setOpacity(0);
                                popup->setTouchEnabled(false);
                                popup->setScale(SETTING(float, "SETUP_WINDOW_SCALE")); //SETUP_WINDOW_SCALE
                                if (auto layer = popup->m_mainLayer) {
                                    if (Ref a = layer->getChildByType<CCScale9Sprite>(-1)) {
                                        a->setOpacity(125);
                                        a->setColor(ccBLACK);
                                    }
                                    if (Ref a = layer->getChildByType<ListBorders>(-1)) {
                                        a->setVisible(0);
                                    }
                                }
                            }
						}

                    }
                }
            ), nullptr)));

            this->addChild(scroll);
        }

        return true;
    }
};

#include <Geode/modify/CCSprite.hpp>
class $modify(AddMenuGameLayerExt, CCSprite) {
    static CCSprite* create(const char* pszFileName) {
        auto rtn = CCSprite::create(pszFileName);
        if (SETTING(bool, "OVERLAP_ALLGRADBG")) if (std::string(pszFileName) == "GJ_gradientBG.png") queueInMainThread(
            [rtn = Ref(rtn)] {
                if (auto a = rtn->getParent()) {
                    auto game = MenuGameLayer::create();
                    game->setTouchEnabled(0);
                    a->insertAfter(game, rtn);
                }
            }
        );
        return rtn;
    }
};