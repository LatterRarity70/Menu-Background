#include "Geode/Geode.hpp"
#include "Geode/ui/GeodeUI.hpp"
#include "CCGIFAnimatedSprite.hpp" // Include your CCGIFAnimatedSprite header

using namespace geode::prelude;

#define SETTING(type, key_name) Mod::get()->getSettingValue<type>(key_name)

$on_mod(Loaded) {
    auto search_paths = {
        getMod()->getConfigDir().string(),
        getMod()->getSaveDir().string(),
        getMod()->getTempDir().string()
    };
    for (auto entry : search_paths) CCFileUtils::get()->addPriorityPath(entry.c_str());
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

        auto BACKGROUND_FILE = SETTING(std::filesystem::path, "BACKGROUND_FILE").u8string();

        CCNodeRGBA* inital_sprite = CCSprite::create((const char*)BACKGROUND_FILE.c_str());
        inital_sprite = inital_sprite->getUserObject(
            "geode.texture-loader/fallback"
        ) ? CCGIFAnimatedSprite::create((const char*)BACKGROUND_FILE.c_str(), not SETTING(bool, "ASYNC_GIF_PARSING")) : inital_sprite;

        if (inital_sprite) {

            auto size = this->getContentSize();

            auto scroll = ScrollLayer::create(size);
            scroll->setID("background-layer"_spr);

            scroll->m_contentLayer->setPositionX(SETTING(float, "BG_POSX"));
            scroll->m_contentLayer->setPositionY(SETTING(float, "BG_POSY"));

            auto setupSprite = [scroll, size](CCNode* sprite = nullptr)
                {
                    scroll->m_contentLayer->removeChildByID("sprite"_spr);
                    auto BACKGROUND_FILE = SETTING(std::filesystem::path, "BACKGROUND_FILE").u8string();
                    sprite = sprite ? sprite : CCSprite::create((const char*)BACKGROUND_FILE.c_str());
                    sprite = sprite->getUserObject(
                        "geode.texture-loader/fallback"
                    ) ? CCGIFAnimatedSprite::create((const char*)BACKGROUND_FILE.c_str(), not SETTING(bool, "ASYNC_GIF_PARSING")) : sprite;
                    sprite = sprite ? sprite : SimpleTextArea::create("FAILED TO LOAD IMAGE")->getLines()[1];
                    if (sprite) {
                        sprite->setPosition({ size.width * 0.5f, size.height * 0.5f });
                        sprite->setID("sprite"_spr);
                        sprite->runAction(CCRepeatForever::create(CCSpawn::create(CCShow::create(), nullptr)));
                        scroll->m_contentLayer->addChild(sprite);
                    };
                };

            setupSprite(inital_sprite);

            scroll->m_disableHorizontal = 0;
            scroll->m_scrollLimitTop = 9999;
            scroll->m_scrollLimitBottom = 9999;
            scroll->m_cutContent = false;

            auto scroll_bg = CCSprite::create("groundSquare_18_001.png");//geode::createLayerBG();
            scroll_bg->setScale(9999.f);
            scroll_bg->setColor(ccBLACK);
            scroll->setID("bg"_spr);
            scroll->addChild(scroll_bg, -1);

            scroll->runAction(CCRepeatForever::create(CCSpawn::create(CallFuncExt::create(
                [scroll, scroll_bg, setupSprite, this] {
                    auto content = scroll->m_contentLayer;

                    scroll->m_disableMovement = not SETTING(bool, "SETUP_MODE");

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

                    if (not SETTING(bool, "SETUP_MODE")) {

                        content->setPositionX(SETTING(float, "BG_POSX"));
                        content->setPositionY(SETTING(float, "BG_POSY"));

                        if (auto a = scroll->getChildByIDRecursive("menu_top_container"_spr)) a->removeFromParent();

                    }
                    else {

                        Mod::get()->setSettingValue("BG_POSX", content->getPositionX());
                        Mod::get()->setSettingValue("BG_POSY", content->getPositionY());
                        

                        auto menu = scroll->getChildByIDRecursive("menu"_spr);
                        if (!menu) {
                            auto menu_top_container = CCNode::create();
                            menu_top_container->setID("menu_top_container"_spr);
                            scroll->addChildAtPosition(menu_top_container, Anchor::TopLeft, {}, 0);

                            menu = CCMenu::create();
                            menu->setID("menu"_spr);
                            menu->setPosition(CCPointZero + CCPointMake(112.f, 0));
                            menu_top_container->addChild(menu);

                            auto menu_bg = CCSprite::create("groundSquare_18_001.png");
                            menu_bg->setAnchorPoint(CCPointMake(0.f, 0.5f));
                            menu_bg->setPosition(CCPointMake(39.f, 0.f));
                            menu_bg->setScaleX(1.675f);
                            menu_bg->setScaleY(1.325f);
                            menu_bg->setOpacity(90);
                            menu_bg->setColor(ccBLACK);
                            menu_bg->setID("menu_bg"_spr);
                            menu_top_container->addChild(menu_bg, -10);

                            auto btnImage = [](std::string text) {
                                auto bs = ButtonSprite::create(text.c_str());
                                bs->setCascadeColorEnabled(1);
                                bs->m_label->setFntFile("chatFont.fnt");
                                bs->m_BGSprite->setSpriteFrame(CCSprite::create("groundSquare_18_001.png")->displayFrame());
                                bs->m_BGSprite->runAction(CCRepeatForever::create(CCTintTo::create(0.f, 0, 0, 0)));
                                bs->m_BGSprite->setOpacity(90);
                                bs->m_BGSprite->setContentSize(bs->m_label->getContentSize() + CCSizeMake(3, 3));
                                bs->setContentSize(bs->m_BGSprite->getContentSize());
                                bs->m_BGSprite->setPosition(bs->getContentSize() / 2);
                                bs->m_label->setPosition(bs->getContentSize() / 2);
                                bs->setScale(0.6f);
                                bs->setID("btn-image."_spr + text);
                                return bs;
                                };
                            auto btn = [](CCMenuItemSpriteExtra* item, bool repeated = false)
                                {
                                    item->setCascadeColorEnabled(1);
                                    item->m_colorEnabled = (1);
                                    item->m_scaleMultiplier = 0.9;
                                    //item->m_animationEnabled = (0);
                                    if (repeated) {
                                        item->getNormalImage()->runAction(CCRepeatForever::create(CCSpawn::create(CallFuncExt::create(
                                            [item] () {
                                                //log::error("{}", item);
                                                if (item->m_bSelected) {
                                                    item->setTag(item->getTag() + 1);
                                                    (item->m_pListener->*item->m_pfnSelector)(item);
                                                }
                                                else item->setTag(0);
                                            }
                                        ), CCDelayTime::create(0.05f), nullptr)));
                                    }
                                    item->setID("btn."_spr + string::replace(item->getNormalImage()->getID(), ""_spr, ""));
                                    return item;
                                };

                            menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                btnImage("   HIDE MENU   "), [this](CCMenuItemSpriteExtra* item) {

                                    if (this->getPositionX()) {
                                        if (auto a = item->getNormalImage()->getChildByType<CCLabelBMFont>(-1))
                                            a->setString("   HIDE MENU   ");
                                        this->getParent()->setPositionX(this->getParent()->getPositionX() - this->getParent()->getContentWidth() * 3);
                                        this->setPositionX(this->getPositionX() + this->getContentWidth() * 3);
                                    }
                                    else {
                                        if (auto a = item->getNormalImage()->getChildByType<CCLabelBMFont>(-1))
                                            a->setString("SHOW MENU");
                                        this->getParent()->setPositionX(this->getParent()->getContentWidth() * 3);
                                        this->setPositionX(this->getContentWidth() * -3);
                                    };
                                }
                            )), Anchor::BottomLeft, { -34.f, -10.f }, 0);

                            menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                btnImage("       RESET       "), [content](CCMenuItemSpriteExtra* item) {
                                    content->setPosition(CCPointZero);
                                }
                            )), Anchor::BottomLeft, { -34.f, -22.f }, 0);
                            
                            //BG_SCALE
                            {

                                //x

                                menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                    btnImage("<"), [](CCMenuItemSpriteExtra* item) {
                                        Mod::get()->setSettingValue(
                                            "BG_SCALEX", SETTING(float, "BG_SCALEX") - 0.0001 - ((double)item->getTag() / 10000)
                                        );
                                    }), true), Anchor::BottomLeft, { (-60.f - 2.0f), -34.f }, 0);

                                menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                    btnImage(" x "), [](CCMenuItemSpriteExtra* item) {
                                        Mod::get()->setSettingValue("BG_SCALEX", 0.0);
                                    }
                                )), Anchor::BottomLeft, { (-49.f - 2.0f), -34.f }, 0);

                                menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                    btnImage(">"), [](CCMenuItemSpriteExtra* item) {
                                        Mod::get()->setSettingValue(
                                            "BG_SCALEX", SETTING(float, "BG_SCALEX") + 0.0001 + ((double)item->getTag() / 10000)
                                        );
                                    }), true), Anchor::BottomLeft, { (-38.f - 2.0f), -34.f }, 0);

                                //y

                                menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                    btnImage("<"), [](CCMenuItemSpriteExtra* item) {
                                        Mod::get()->setSettingValue(
                                            "BG_SCALEY", SETTING(float, "BG_SCALEY") - 0.0001 - ((double)item->getTag() / 10000)
                                        );
                                    }), true), Anchor::BottomLeft, { (-60.f - 2.0f) + (60.f - 26.f), -34.f }, 0);

                                menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                    btnImage(" y "), [](CCMenuItemSpriteExtra* item) {
                                        Mod::get()->setSettingValue("BG_SCALEY", 0.0);
                                    }
                                )), Anchor::BottomLeft, { (-49.f - 2.0f) + (60.f - 26.f), -34.f }, 0);

                                menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                    btnImage(">"), [](CCMenuItemSpriteExtra* item) {
                                        Mod::get()->setSettingValue(
                                            "BG_SCALEY", SETTING(float, "BG_SCALEY") + 0.0001 + ((double)item->getTag() / 10000)
                                        );
                                    }), true), Anchor::BottomLeft, { (-38.f - 2.0f) + (60.f - 26.f), -34.f }, 0);
                            };

                            menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                btnImage("      SWITCH      "), [content](CCMenuItemSpriteExtra* item) {
                                    Mod::get()->setSettingValue("BG_SCALEX", 0.f);
                                    Mod::get()->setSettingValue("BG_SCALEY", 0.f);
                                    auto setting = getMod()->getSetting("BG_SCALE_TYPE");
                                    auto node = setting->createNode(10.f);
                                    if (auto a = node->getButtonMenu()->getChildByType<CCMenuItemSpriteExtra*>(-1)) a->activate();
                                    node->commit();
                                }
                            )), Anchor::BottomLeft, { -34.f, -46.000 }, 0);

                            //BG_ZORDER
                            {
                                menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                    btnImage("-          "), [](CCMenuItemSpriteExtra* item) {
                                        Mod::get()->setSettingValue(
                                            "BG_ZORDER", SETTING(int, "BG_ZORDER") - 1 - (int)(item->getTag() / 10)
                                        );
                                    }), true), Anchor::BottomLeft, { -51.f, -58.000f }, 0);

                                menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                    btnImage("          +"), [](CCMenuItemSpriteExtra* item) {
                                        Mod::get()->setSettingValue(
                                            "BG_ZORDER", SETTING(int, "BG_ZORDER") + 1 + (int)(item->getTag() / 10)
                                        );
                                    }), true), Anchor::BottomLeft, { -17.f, -58.000f }, 0);
                            };

                            menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                btnImage(" TOGGLE HIDE_GROUND "), [scroll](CCMenuItemSpriteExtra* item) {
                                    Mod::get()->setSettingValue("HIDE_GROUND", !SETTING(bool, "HIDE_GROUND"));
                                }
                            )), Anchor::BottomLeft, { -14.000f, -72.000f }, 0);

                            menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                btnImage(" TOGGLE HIDE_GAME "), [scroll](CCMenuItemSpriteExtra* item) {
                                    Mod::get()->setSettingValue("HIDE_GAME", !SETTING(bool, "HIDE_GAME"));
                                }
                            )), Anchor::BottomLeft, { 91.000f, -72.000f }, 0);

                            menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                btnImage("SETTINGS"), [scroll](CCMenuItemSpriteExtra* item) {
                                    openSettingsPopup(getMod(), 1);
                                }
                            )), Anchor::BottomLeft, { 115.500f, -59.00f }, 0);

                            menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                btnImage(" X "), [scroll](CCMenuItemSpriteExtra* item) {
                                    Mod::get()->setSettingValue("SETUP_MODE", !SETTING(bool, "SETUP_MODE"));
                                }
                            )), Anchor::BottomLeft, { 130.000f, -10.000f }, 0);

                            auto apply_file_btn = [btn, btnImage, setupSprite](SettingNodeV3* node)
                                {
                                    return btn(CCMenuItemExt::createSpriteExtra(
                                        btnImage("APPLY FILE"), [node, setupSprite](CCMenuItemSpriteExtra* item) {
                                            node->commit();
                                            node->removeFromParent();
                                            item->removeFromParent();
                                            setupSprite();
                                        }
                                    ));
                                };

                            menu->addChildAtPosition(btn(CCMenuItemExt::createSpriteExtra(
                                btnImage(" + "), [menu, apply_file_btn](CCMenuItemSpriteExtra* item) {

                                    auto setting = getMod()->getSetting("BACKGROUND_FILE");
                                    auto node = setting->createNode(310.f);
                                    if (auto a = node->getButtonMenu()->getChildByType<CCMenuItemSpriteExtra*>(-1)) a->activate();

                                    menu->removeChildByID("btn.btn-image.APPLY FILE"_spr);
                                    menu->addChildAtPosition(node, Anchor::BottomLeft, { 9999.000f, 9999.000f }, 0);
                                    menu->addChildAtPosition(apply_file_btn(node), Anchor::BottomLeft, { 98.000f, -11.000f}, 0);
                                }
                            )), Anchor::BottomLeft, { 130.000f, -22.000f }, 0);

                        };

                        menu->removeChildByID("inflbl");

                        auto inflbl = CCLabelBMFont::create(fmt::format(
                            """" "SETUP_MODE"
                            "\n" "BG_POS: {:0.4}, {:0.4}"
                            "\n" "BG_SCALE: {:0.4}, {:0.4}"
                            "\n" "or {}"
                            "\n" "BG_ZORDER: {}"
                            , SETTING(float, "BG_POSX")
                            , SETTING(float, "BG_POSY")
                            , SETTING(float, "BG_SCALEX")
                            , SETTING(float, "BG_SCALEY")
                            , SETTING(std::string, "BG_SCALE_TYPE")
                            , SETTING(int, "BG_ZORDER")
                        ).c_str(), "chatFont.fnt");
                        inflbl->setAlignment(CCTextAlignment::kCCTextAlignmentLeft);
                        inflbl->setAnchorPoint({ 0.0f, 1.060f });
                        inflbl->setScale(0.745f);

                        inflbl->setID("inflbl");
                        menu->addChild(inflbl);

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
            [rtn] {
                if (auto a = rtn->getParent()) {
                    //missing macos bindings moment
                    auto fuuuuck = new MenuGameLayer();
                    if (fuuuuck && fuuuuck->MenuGameLayer::init()) {
                        fuuuuck->autorelease();
                        a->insertAfter(fuuuuck, rtn);
                    }
                    else CC_SAFE_DELETE(fuuuuck);
                }
            }
        );
        return rtn;
    }
};