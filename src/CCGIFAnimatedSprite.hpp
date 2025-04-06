#include "Geode/Geode.hpp"
#include <gif_lib.h>

using namespace geode::prelude;

struct GifFrame {
    CCTexture2D* texture;
    float delay;
};

class GifParser {
public:
    using FrameCallback = std::function<void(const GifFrame&)>;

    GifParser(const std::string& filename, FrameCallback callback)
        : m_filename(filename), m_callback(callback) {
    }

    void start(bool notAsync = false) {
        auto lb = [this]()
            {
                int error = 0;
                GifFileType* gifFile = DGifOpenFileName(m_filename.c_str(), &error);
                if (!gifFile) return;
                if (error or (DGifSlurp(gifFile) == GIF_ERROR)) {
                    DGifCloseFile(gifFile);
                    return;
                }
                int width = gifFile->SWidth;
                int height = gifFile->SHeight;
                std::vector<unsigned char> canvas(width * height * 4, 0);

                for (int i = 0; i < gifFile->ImageCount; i++) {
                    auto savedImage = &gifFile->SavedImages[i];
                    GraphicsControlBlock gcb;
                    DGifSavedExtensionToGCB(gifFile, i, &gcb);
                    float delay = gcb.DelayTime / 100.0f;
                    int transparentColorIndex = (gcb.TransparentColor == NO_TRANSPARENT_COLOR) ? -1 : gcb.TransparentColor;

                    if (gcb.DisposalMode == DISPOSE_BACKGROUND) {
                        for (int y = savedImage->ImageDesc.Top; y < savedImage->ImageDesc.Top + savedImage->ImageDesc.Height; y++) {
                            for (int x = savedImage->ImageDesc.Left; x < savedImage->ImageDesc.Left + savedImage->ImageDesc.Width; x++) {
                                int index = (y * width + x) * 4;
                                canvas[index + 0] = 0;
                                canvas[index + 1] = 0;
                                canvas[index + 2] = 0;
                                canvas[index + 3] = 0;
                            }
                        }
                    }

                    ColorMapObject* colorMap = savedImage->ImageDesc.ColorMap ? savedImage->ImageDesc.ColorMap : gifFile->SColorMap;
                    if (!colorMap) continue;

                    for (int y = 0; y < savedImage->ImageDesc.Height; y++) {
                        for (int x = 0; x < savedImage->ImageDesc.Width; x++) {
                            int srcIndex = y * savedImage->ImageDesc.Width + x;
                            int dstX = savedImage->ImageDesc.Left + x;
                            int dstY = savedImage->ImageDesc.Top + y;
                            int dstIndex = (dstY * width + dstX) * 4;
                            int colorIndex = savedImage->RasterBits[srcIndex];
                            if (colorIndex != transparentColorIndex) {
                                GifColorType color = colorMap->Colors[colorIndex];
                                canvas[dstIndex + 0] = color.Red;
                                canvas[dstIndex + 1] = color.Green;
                                canvas[dstIndex + 2] = color.Blue;
                                canvas[dstIndex + 3] = 255;
                            }
                        }
                    }

                    auto canvasCopy = canvas;
                    auto createTextureTask = [canvasCopy, width, height, delay, this]()
                        {
                            CCTexture2D* texture = new CCTexture2D();
                            if (texture->initWithData(canvasCopy.data(), kCCTexture2DPixelFormat_RGBA8888, width, height, CCSizeMake(width, height))) {
                                texture->retain();
                                GifFrame frame;
                                frame.texture = texture;
                                frame.delay = delay;
                                m_callback(frame);
                            }
                            else texture->release();
                        };
                    queueInMainThread(createTextureTask);
                }
                DGifCloseFile(gifFile);
            };
        notAsync ? lb() : std::thread(lb).detach();
    }

    std::string m_filename;
    FrameCallback m_callback;
};

class CCGIFAnimatedSprite : public CCSprite {
public:
    static CCGIFAnimatedSprite* create(const std::string gifFile, bool notAsync = false) {
        CCGIFAnimatedSprite* sprite = new CCGIFAnimatedSprite();
        if (sprite && sprite->initWithGif(gifFile, notAsync)) {
            sprite->autorelease();
            return sprite;
        }
        CC_SAFE_DELETE(sprite);
        return nullptr;
    }

    bool initWithGif(const std::string gifFile, bool notAsync = false) {
        if (!this->init()) return false;
        if (!fileExistsInSearchPaths(gifFile.c_str())) {
            log::error("{} doesn't exists in search paths", gifFile);
            return false;
        }

        auto prew = (gifFile + ".png");
        if (auto a = CCSprite::create(prew.c_str())->displayFrame()) this->setDisplayFrame(a);

        m_currentFrame = 0;
        m_elapsedTime = 0.0f;
        m_loop = true;

        m_asyncParser = new GifParser(gifFile, [this](const GifFrame& frame) {
            this->m_frames.push_back(frame.texture);
            this->m_frameDelays.push_back(frame.delay);
            if (this->m_frames.size() == 1) {
                this->setTexture(frame.texture);
                this->setTextureRect(CCRectMake(0, 0, frame.texture->getContentSize().width, frame.texture->getContentSize().height));
            }
            });
        m_asyncParser->start(notAsync);

        this->scheduleUpdate();
        return true;
    }

    virtual void update(float dt) override {

        if (m_frames.empty()) return;

        m_elapsedTime += dt;
        if (m_elapsedTime >= m_frameDelays[m_currentFrame]) {
            m_elapsedTime = 0.0f;
            m_currentFrame++;
            if (m_currentFrame >= m_frames.size()) {
                if (m_loop) {
                    m_currentFrame = 0;
                }
                else {
                    m_currentFrame = m_frames.size() - 1;
                    this->unscheduleUpdate();
                    return;
                }
            }
            this->setTexture(m_frames[m_currentFrame]);
            this->setTextureRect(CCRectMake(0, 0, m_frames[m_currentFrame]->getContentSize().width, m_frames[m_currentFrame]->getContentSize().height));
        }
    }

protected:
    CCGIFAnimatedSprite()
        : m_currentFrame(0)
        , m_elapsedTime(0.0f)
        , m_loop(true)
        , m_asyncParser(nullptr) {
    }

    virtual ~CCGIFAnimatedSprite() {
        for (auto texture : m_frames) {
            texture->release();
        }
        delete m_asyncParser;
    }

private:
    std::vector<CCTexture2D*> m_frames;
    std::vector<float> m_frameDelays;
    size_t m_currentFrame;
    float m_elapsedTime;
    bool m_loop;
    GifParser* m_asyncParser;
};
