/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecDRMPRIME.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererDRMPRIME.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererDRMPRIMEGLES.h"

#include "cores/RetroPlayer/process/gbm/RPProcessInfoGbm.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererGBM.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererOpenGLES.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"

#include "WinSystemGbmGLESContext.h"
#include "OptionalsReg.h"
#include "utils/log.h"

#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

using namespace KODI;

CWinSystemGbmGLESContext::CWinSystemGbmGLESContext()
: m_pGLContext{EGL_PLATFORM_GBM_MESA, "EGL_MESA_platform_gbm"}
{}

std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem()
{
  std::unique_ptr<CWinSystemBase> winSystem(new CWinSystemGbmGLESContext());
  return winSystem;
}

bool CWinSystemGbmGLESContext::InitWindowSystem()
{
  CLinuxRendererGLES::Register();
  RETRO::CRPProcessInfoGbm::Register();
  RETRO::CRPProcessInfoGbm::RegisterRendererFactory(new RETRO::CRendererFactoryGBM);
  RETRO::CRPProcessInfoGbm::RegisterRendererFactory(new RETRO::CRendererFactoryOpenGLES);

  if (!CWinSystemGbm::InitWindowSystem())
  {
    return false;
  }

  if (!m_pGLContext.CreatePlatformDisplay(m_GBM->GetDevice(), m_GBM->GetDevice(), EGL_OPENGL_ES2_BIT, EGL_OPENGL_ES_API))
  {
    return false;
  }

  const EGLint contextAttribs[] =
  {
    EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
  };

  if (!m_pGLContext.CreateContext(contextAttribs))
  {
    return false;
  }

  bool general, deepColor;
  m_vaapiProxy.reset(GBM::VaapiProxyCreate());
  GBM::VaapiProxyConfig(m_vaapiProxy.get(), m_pGLContext.GetEGLDisplay());
  GBM::VAAPIRegisterRender(m_vaapiProxy.get(), general, deepColor);

  if (general)
  {
    GBM::VAAPIRegister(m_vaapiProxy.get(), deepColor);
  }

  CRendererDRMPRIMEGLES::Register();
  CRendererDRMPRIME::Register();
  CDVDVideoCodecDRMPRIME::Register();

  return true;
}

bool CWinSystemGbmGLESContext::DestroyWindowSystem()
{
  CDVDFactoryCodec::ClearHWAccels();
  VIDEOPLAYER::CRendererFactory::ClearRenderer();

  m_pGLContext.Destroy();

  return CWinSystemGbm::DestroyWindowSystem();
}

bool CWinSystemGbmGLESContext::CreateNewWindow(const std::string& name,
                                               bool fullScreen,
                                               RESOLUTION_INFO& res)
{
  m_pGLContext.DestroySurface();

  if (!CWinSystemGbm::DestroyWindow())
  {
    return false;
  }

  if (!CWinSystemGbm::CreateNewWindow(name, fullScreen, res))
  {
    return false;
  }

  if (!m_pGLContext.CreatePlatformSurface(m_GBM->GetSurface(), m_GBM->GetSurface()))
  {
    return false;
  }

  if (!m_pGLContext.BindContext())
  {
    return false;
  }

  return true;
}

bool CWinSystemGbmGLESContext::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  if (res.iWidth != m_nWidth ||
      res.iHeight != m_nHeight)
  {
    CLog::Log(LOGDEBUG, "CWinSystemGbmGLESContext::%s - resolution changed, creating a new window", __FUNCTION__);
    CreateNewWindow("", fullScreen, res);
  }

  m_pGLContext.SwapBuffers();

  CWinSystemGbm::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight);

  if (!m_delayDispReset)
  {
    CSingleLock lock(m_resourceSection);

    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }

  return true;
}

void CWinSystemGbmGLESContext::PresentRender(bool rendered, bool videoLayer)
{
  if (!m_bRenderCreated)
    return;

  if (rendered || videoLayer)
  {
    if (rendered)
      m_pGLContext.SwapBuffers();
    CWinSystemGbm::FlipPage(rendered, videoLayer);
  }
  else
  {
    CWinSystemGbm::WaitVBlank();
  }

  if (m_delayDispReset && m_dispResetTimer.IsTimePast())
  {
    m_delayDispReset = false;
    CSingleLock lock(m_resourceSection);

    for (std::vector<IDispResource *>::iterator i = m_resources.begin(); i != m_resources.end(); ++i)
      (*i)->OnResetDisplay();
  }
}

void CWinSystemGbmGLESContext::delete_CVaapiProxy::operator()(CVaapiProxy *p) const
{
  GBM::VaapiProxyDelete(p);
}

EGLDisplay CWinSystemGbmGLESContext::GetEGLDisplay() const
{
  return m_pGLContext.GetEGLDisplay();
}

EGLSurface CWinSystemGbmGLESContext::GetEGLSurface() const
{
  return m_pGLContext.GetEGLSurface();
}

EGLContext CWinSystemGbmGLESContext::GetEGLContext() const
{
  return m_pGLContext.GetEGLContext();
}

EGLConfig  CWinSystemGbmGLESContext::GetEGLConfig() const
{
  return m_pGLContext.GetEGLConfig();
}
