#include "render_textute.hpp"

using namespace cocos2d;

RenderTexture::~RenderTexture() {
    cleanup();
}

bool RenderTexture::init(unsigned width, unsigned height) {
    if (m_initialized) {
        cleanup();
    }

    m_width = width;
    m_height = height;

    GLint old_fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &old_fbo);

    m_texture = new CCTexture2D();
    if (!m_texture) return false;

    bool success = m_texture->initWithData(
        nullptr, 
        kCCTexture2DPixelFormat_RGB888, 
        m_width, 
        m_height, 
        CCSize(static_cast<float>(m_width), static_cast<float>(m_height))
    );

    if (!success) {
        m_texture->release();
        m_texture = nullptr;
        return false;
    }

    m_texture->setAliasTexParameters();

    glGenFramebuffersEXT(1, &m_fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_texture->getName(), 0);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, old_fbo);
    m_initialized = true;

    return true;
}

void RenderTexture::cleanup() {
    if (!m_initialized) return;

    if (m_fbo) {
        glDeleteFramebuffersEXT(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_texture) {
        m_texture->release();
        m_texture = nullptr;
    }
    m_width = 0;
    m_height = 0;
    m_initialized = false;
}

void RenderTexture::capture(cocos2d::CCNode* target_node, std::mutex& lock, std::vector<uint8_t>& data, volatile bool& frame_has_data) {
    if (!m_initialized || !target_node) return;

    GLint old_fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &old_fbo);

    glViewport(0, 0, m_width, m_height);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    target_node->visit();

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    
    lock.lock();
    frame_has_data = true;
    glReadPixels(0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    lock.unlock();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, old_fbo);
    CCDirector::sharedDirector()->setViewport();
}