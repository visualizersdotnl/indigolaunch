
// indigolaunch -- basic 2D renderer

#pragma once

#include "renderutil.h"

// forward decl.
class Font;
class TextRect;

class Renderer
{
	// hack!
	friend class Font;
	friend class TextRect;

public:
	// resource: Texture2D
	class Texture2D
	{
		friend class Renderer;
		friend class TextRect; // hack!

	public:
		// for SAFE_RELEASE()
		void Release() { delete this; }
		
		float GetWidth() const { return (float) m_Desc.Width; }
		float GetHeight() const { return (float) m_Desc.Height; }
		float GetAspectRatio() const { return GetWidth() / GetHeight(); }
	
	private:
		Texture2D(ID3D10Texture2D *pResource, 
		          ID3D10ShaderResourceView *pSRV, ID3D10RenderTargetView *pRTV = NULL) : 
			m_pResource(pResource),
			m_pSRV(pSRV),
			m_pRTV(pRTV)
		{
			ASSERT(NULL != pResource);
			ASSERT(NULL != pSRV);
			pResource->GetDesc(&m_Desc);
		}
		
		~Texture2D() 
		{
			SAFE_RELEASE(m_pSRV);
			SAFE_RELEASE(m_pRTV);
			SAFE_RELEASE(m_pResource);	
		}

		ID3D10Texture2D          *m_pResource;
		ID3D10ShaderResourceView *m_pSRV;
		ID3D10RenderTargetView   *m_pRTV;
		
		D3D10_TEXTURE2D_DESC m_Desc;
	};

	// state: blend mode
	enum BlendMode
	{
		kOpaque,
		kAlpha,
		kAdditive,
		kSubtractive,
		kNumBlendModes
	};

	Renderer(ID3D10Device &D3D, IDXGISwapChain &swapChain);
	~Renderer();

private:
	// bind & release swap chain's back buffer (once exposed for WM_SIZE)
	void BindBackBuffer();
	void ReleaseBackBuffer();

public:	
	bool Create();
	
	// - resolution adapted from image (scaled to fit in extreme cases)
	// - format forced to 32-bit RGBA (DXGI_R8G8B8A8_UNORM)
	Texture2D *CreateTextureFromFile(const std::wstring &path, bool noMipLevels);

	// - format forced to 32-bit RGBA (DXGI_R8G8B8A8_UNORM)
	// - currently only used for TextRect (hack)
	Texture2D *CreateRenderTarget(unsigned int width, unsigned int height);
	
	// see CompileShader() below for details
	ID3D10VertexShader *CompileVertexShader(const std::wstring &path, ID3D10Blob **ppBytecode = NULL);
	ID3D10PixelShader *CompilePixelShader(const std::wstring &path);
	
	void BeginFrame(const D3DXCOLOR &clearColor);
	void EndFrame();

	// - sprites are drawn in a 1920*1080 (16:9) top-left aligned 2D space
	// - sprites are automatically flushed on EndFrame()
	void AddSprite(
		const Texture2D *pTexture, 
		BlendMode blendMode,
		const D3DXCOLOR &color,
		const D3DXVECTOR2 &topLeft, 
		const D3DXVECTOR2 &size, 
		float sortZ,
		float rotateZ = 0.f,
		ID3D10PixelShader *pPS = NULL);

	// simplified AddSprite()
	void AddSprite(
		const Texture2D *pTexture, 
		BlendMode blendMode,
		const D3DXVECTOR2 &topLeft, 
		float sortZ,
		float alpha = 1.f)
	{
		ASSERT(NULL != pTexture);
		const D3DXVECTOR2 size(pTexture->GetWidth(), pTexture->GetHeight());
		AddSprite(pTexture, blendMode, D3DXCOLOR(1.f, 1.f, 1.f, alpha), topLeft, size, sortZ);
	}

	// AddSprite() simplified & centered
	void AddSpriteCenter(
		const Texture2D *pTexture, 
		BlendMode blendMode,
		const D3DXVECTOR2 &center, 
		float sortZ,
		float alpha = 1.f)
	{
		ASSERT(NULL != pTexture);
		const D3DXVECTOR2 size(pTexture->GetWidth(), pTexture->GetHeight());
		const D3DXVECTOR2 topLeft = center - size*0.5f;
		AddSprite(pTexture, blendMode, D3DXCOLOR(1.f, 1.f, 1.f, alpha), topLeft, size, sortZ);
	}

	// full screen fade sprite
	void FadeSprite(
		float sortZ,
		float alpha)
	{
		AddSprite(
			NULL, 
			kAlpha,
			D3DXCOLOR(0.f, 0.f, 0.f, alpha),
			D3DXVECTOR2(0.f, 0.f),
			D3DXVECTOR2(1920.f, 1080.f), 
			sortZ);
	}

	// ribbon (background) effect
	// call only once! (see impl.)
	void DrawRibbon(float sortZ, float alpha, float time);

private:
	// - shader model 4.0
	// - entry point function: main()
	// - #include not supported
	// - warnings treated as errors
	// - compatibility issue: may not work on systems with an outdated DirectX distribution
	bool CompileShader(const std::wstring &path, const std::string &profile, ID3D10Blob **ppBytecode);

	ID3D10VertexShader *CreateVertexShader(const BYTE *pBytecode, size_t size);
	ID3D10PixelShader *CreatePixelShader(const BYTE *pBytecode, size_t size);

private:
	void FlushSprites();

	ID3D10Device &m_D3D;
	IDXGISwapChain &m_swapChain;
	DXGI_SWAP_CHAIN_DESC m_swapChainDesc;

	ID3D10Texture2D *m_pBackBuffer;
	ID3D10RenderTargetView *m_pBackBufferRTV;
	D3D10_VIEWPORT m_fullVP, m_adjVP;

	Texture2D *m_pWhiteTex;

	ID3D10Buffer *m_pSpriteVB;
	ID3D10VertexShader *m_pSpriteVS;
//	ID3D10Blob *m_pSpriteVSBytecode;
	ID3D10PixelShader *m_pSpritePS;
	ID3D10InputLayout *m_pSpriteVertexLayout;

	ID3D10PixelShader *m_pRibbonPS;
	ID3D10Buffer *m_pRibbonConst;

	ID3D10SamplerState *m_pDefSamplerState;
	ID3D10BlendState *m_pBlendStates[kNumBlendModes];

	bool m_isInFrame;
	
	struct Sprite
	{
		ID3D10PixelShader *pPS;
		ID3D10ShaderResourceView *pTextureSRV;
		BlendMode blendMode;
		float sortZ;
		unsigned int vertexOffs;

		// std::sort() predicate
		bool operator <(const Sprite &RHS) const {
			return sortZ < RHS.sortZ;
		}
	};
	
	std::list<Sprite> m_sprites;

	struct SpriteVertex
	{
		D3DXVECTOR3 position;
		uint32_t ARGB;
		D3DXVECTOR2 UV;
	} *m_pSpriteVertices;
};
