
// indigolaunch -- basic 2D renderer

#include "platform.h"
#include "renderer.h"

// hack: precompiled shaders
#include "..\target\shaders\2Dsprite.vsh"
#include "..\target\shaders\2Dsprite.psh"
#include "..\target\shaders\ribbon.psh"

const float kAspectRatio = 16.f/9.f;
const unsigned int kMaxSprites = 4096;

Renderer::Renderer(ID3D10Device &D3D, IDXGISwapChain &swapChain) :
	m_D3D(D3D), 
	m_swapChain(swapChain),
	m_pBackBuffer(NULL),
	m_pBackBufferRTV(NULL),
	m_pWhiteTex(NULL),
	m_pSpriteVB(NULL),
	m_pSpriteVS(NULL),
//	m_pSpriteVSBytecode(NULL),
	m_pSpritePS(NULL),
	m_pSpriteVertexLayout(NULL),
	m_pRibbonPS(NULL),
	m_pRibbonConst(NULL),
	m_pDefSamplerState(NULL),
	m_isInFrame(false),
	m_pSpriteVertices(NULL)
{
	m_swapChain.GetDesc(&m_swapChainDesc);
	memset(m_pBlendStates, 0, kNumBlendModes*sizeof(ID3D10BlendState)); // C++11: move to initializer list!
}

Renderer::~Renderer()
{
	SAFE_RELEASE(m_pWhiteTex);

	SAFE_RELEASE(m_pSpriteVB);
	SAFE_RELEASE(m_pSpriteVS);
//	SAFE_RELEASE(m_pSpriteVSBytecode);
	SAFE_RELEASE(m_pSpritePS);
	SAFE_RELEASE(m_pSpriteVertexLayout);

	SAFE_RELEASE(m_pRibbonPS);
	SAFE_RELEASE(m_pRibbonConst);

	for (int iState = 0; iState < kNumBlendModes; ++iState)
		SAFE_RELEASE(m_pBlendStates[iState]);

	SAFE_RELEASE(m_pDefSamplerState);
	
	ReleaseBackBuffer();
}

void Renderer::BindBackBuffer()
{
	ReleaseBackBuffer();

	// set back buffer as render target
	m_swapChain.GetBuffer(0, __uuidof(ID3D10Texture2D), reinterpret_cast<void **>(&m_pBackBuffer));
	m_D3D.CreateRenderTargetView(m_pBackBuffer, NULL, &m_pBackBufferRTV);
	m_D3D.OMSetRenderTargets(1, &m_pBackBufferRTV, NULL);

	// define full & adjusted (16:9) viewport
	m_fullVP.TopLeftX = 0;
	m_fullVP.TopLeftY = 0;
	m_fullVP.Width = m_swapChainDesc.BufferDesc.Width;
	m_fullVP.Height = m_swapChainDesc.BufferDesc.Height;
	m_fullVP.MinDepth = 0.f;
	m_fullVP.MaxDepth = 1.f;

	const float fullAspectRatio = (float) m_fullVP.Width / m_fullVP.Height;
	unsigned int xResAdj, yResAdj;
	if (fullAspectRatio < kAspectRatio)
	{
		const float scale = fullAspectRatio / kAspectRatio;
		xResAdj = m_fullVP.Width;
		yResAdj = (unsigned int) (m_fullVP.Height*scale);
	}
	else if (fullAspectRatio > kAspectRatio)
	{
		const float scale = kAspectRatio / fullAspectRatio;
		xResAdj = (unsigned int) (m_fullVP.Width*kAspectRatio);
		yResAdj = m_fullVP.Height;
	}
	else // ==
	{
		xResAdj = m_fullVP.Width;
		yResAdj = m_fullVP.Height;
	}
	
	m_adjVP.Width = xResAdj;
	m_adjVP.Height = yResAdj;
	m_adjVP.TopLeftX = (m_fullVP.Width-xResAdj)/2;
	m_adjVP.TopLeftY = (m_fullVP.Height-yResAdj)/2;
	m_adjVP.MinDepth = 0.f;
	m_adjVP.MaxDepth = 1.f;
}

void Renderer::ReleaseBackBuffer()
{
	SAFE_RELEASE(m_pBackBufferRTV);
	SAFE_RELEASE(m_pBackBuffer);
	m_pBackBufferRTV = NULL;
	m_pBackBuffer = NULL;
}

bool Renderer::Create()
{
	BindBackBuffer();

	// create default (white) texture
	{
		D3D10_TEXTURE2D_DESC texDesc;
		texDesc.Width = 4;
		texDesc.Height = 4;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D10_USAGE_DEFAULT;
		texDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = 0;
		texDesc.MiscFlags = 0;

		uint32_t whitePixels[4*4];
		memset(whitePixels, 0xff, 4*4*sizeof(uint32_t));

		D3D10_SUBRESOURCE_DATA data;
		data.pSysMem = whitePixels;
		data.SysMemPitch = 4*sizeof(uint32_t);
		data.SysMemSlicePitch = 0;

		ID3D10Texture2D *pResource;
		VERIFY(S_OK == m_D3D.CreateTexture2D(&texDesc, &data, &pResource));

		ID3D10ShaderResourceView *pView;
		VERIFY(S_OK == m_D3D.CreateShaderResourceView(pResource, NULL, &pView));

		m_pWhiteTex = new Texture2D(pResource, pView);
	}

	// create sprite vertex buffer
	D3D10_BUFFER_DESC bufferDesc;
	bufferDesc.ByteWidth = kMaxSprites*6*sizeof(SpriteVertex);
	bufferDesc.Usage = D3D10_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;

	if FAILED(m_D3D.CreateBuffer(&bufferDesc, NULL, &m_pSpriteVB))
	{
		SetLastError(L"Can not create dynamic sprite vertex buffer (" + ToString(bufferDesc.ByteWidth) + L" bytes).");
		return false;
	}

	// create ribbon shader & constant buffer
	m_pRibbonPS = CreatePixelShader(g_PS_ribbon, _countof(g_PS_ribbon));

	D3D10_BUFFER_DESC bufDesc;
	bufDesc.ByteWidth = 16; // sizeof(float);
	bufDesc.Usage = D3D10_USAGE_DYNAMIC;
	bufDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
	bufDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	bufDesc.MiscFlags = 0;
	VERIFY(S_OK == m_D3D.CreateBuffer(&bufDesc, NULL, &m_pRibbonConst));

	// create default sprite shaders
	m_pSpriteVS = CreateVertexShader(g_VS_2Dsprite, _countof(g_VS_2Dsprite));
	m_pSpritePS = CreatePixelShader(g_PS_2Dsprite, _countof(g_PS_2Dsprite));

	// create sprite vertex layout
	D3D10_INPUT_ELEMENT_DESC elemDesc[3];
	elemDesc[0].SemanticName = "SV_POSITION";
	elemDesc[0].SemanticIndex = 0;
	elemDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elemDesc[0].InputSlot = 0;
	elemDesc[0].AlignedByteOffset = D3D10_APPEND_ALIGNED_ELEMENT;
	elemDesc[0].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
	elemDesc[0].InstanceDataStepRate = 0;
	elemDesc[1].SemanticName = "COLOR";
	elemDesc[1].SemanticIndex = 0;
	elemDesc[1].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	elemDesc[1].InputSlot = 0;
	elemDesc[1].AlignedByteOffset = D3D10_APPEND_ALIGNED_ELEMENT;
	elemDesc[1].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
	elemDesc[1].InstanceDataStepRate = 0;
	elemDesc[2].SemanticName = "TEXCOORD";
	elemDesc[2].SemanticIndex = 0;
	elemDesc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	elemDesc[2].InputSlot = 0;
	elemDesc[2].AlignedByteOffset = D3D10_APPEND_ALIGNED_ELEMENT;
	elemDesc[2].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
	elemDesc[2].InstanceDataStepRate = 0;

	if FAILED(m_D3D.CreateInputLayout(
		elemDesc,
		3,
		g_VS_2Dsprite,           // m_pSpriteVSBytecode->GetBufferPointer(),
		_countof(g_VS_2Dsprite), // m_pSpriteVSBytecode->GetBufferSize(),
		&m_pSpriteVertexLayout))
	{
		ASSERT(0); // review input layout & shader: shaders\2Dsprite.vs
		SetLastError(L"Run-time sprite vertex layout does not match shader: 'shaders\\2Dsprite.vs'");
		return false;
	}

	// create default sampler state (for 2D image)
	D3D10_SAMPLER_DESC samplerDesc;
	samplerDesc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.f;
	samplerDesc.MaxAnisotropy = 4;
	samplerDesc.ComparisonFunc = D3D10_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0.f;
	samplerDesc.BorderColor[1] = 0.f;
	samplerDesc.BorderColor[2] = 0.f;
	samplerDesc.BorderColor[3] = 0.f;
	samplerDesc.MinLOD = 0.f;
	samplerDesc.MaxLOD = FLT_MAX;
	m_D3D.CreateSamplerState(&samplerDesc, &m_pDefSamplerState);

	// create non-default blend state(s)
	D3D10_BLEND_DESC alphaDesc;
	alphaDesc.AlphaToCoverageEnable = FALSE;
	alphaDesc.BlendEnable[0] = TRUE;
	memset(alphaDesc.BlendEnable+1, FALSE, 7*sizeof(BOOL));
	alphaDesc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
	alphaDesc.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
	alphaDesc.BlendOp = D3D10_BLEND_OP_ADD;
	alphaDesc.SrcBlendAlpha = D3D10_BLEND_ONE;
	alphaDesc.DestBlendAlpha = D3D10_BLEND_ZERO;
	alphaDesc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
	memset(alphaDesc.RenderTargetWriteMask, D3D10_COLOR_WRITE_ENABLE_ALL, 8*sizeof(UINT8));
	m_D3D.CreateBlendState(&alphaDesc, &m_pBlendStates[kAlpha]);

	D3D10_BLEND_DESC addDesc;
	addDesc.AlphaToCoverageEnable = FALSE;
	addDesc.BlendEnable[0] = TRUE;
	memset(addDesc.BlendEnable+1, FALSE, 7*sizeof(BOOL));
	addDesc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
	addDesc.DestBlend = D3D10_BLEND_ONE;	
	addDesc.BlendOp = D3D10_BLEND_OP_ADD;
	addDesc.SrcBlendAlpha = D3D10_BLEND_ONE;
	addDesc.DestBlendAlpha = D3D10_BLEND_ZERO;
	addDesc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
	memset(addDesc.RenderTargetWriteMask, D3D10_COLOR_WRITE_ENABLE_ALL, 8*sizeof(UINT8));
	m_D3D.CreateBlendState(&addDesc, &m_pBlendStates[kAdditive]);

	D3D10_BLEND_DESC subDesc;
	subDesc.AlphaToCoverageEnable = FALSE;
	subDesc.BlendEnable[0] = TRUE;
	memset(subDesc.BlendEnable+1, FALSE, 7*sizeof(BOOL));
	subDesc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
	subDesc.DestBlend = D3D10_BLEND_ONE;
	subDesc.BlendOp = D3D10_BLEND_OP_REV_SUBTRACT;
	subDesc.SrcBlendAlpha = D3D10_BLEND_ONE;
	subDesc.DestBlendAlpha = D3D10_BLEND_ZERO;
	subDesc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
	memset(subDesc.RenderTargetWriteMask, D3D10_COLOR_WRITE_ENABLE_ALL, 8*sizeof(UINT8));
	m_D3D.CreateBlendState(&subDesc, &m_pBlendStates[kSubtractive]);

	return true;
}

Renderer::Texture2D *Renderer::CreateTextureFromFile(const std::wstring &path, bool noMipLevels)
{
	ID3D10Texture2D *pResource = NULL;
	ID3D10ShaderResourceView *pView = NULL;

	D3DX10_IMAGE_INFO imageInfo;
	if SUCCEEDED(D3DX10GetImageInfoFromFile(path.c_str(), NULL, &imageInfo, NULL))
	{
		D3DX10_IMAGE_LOAD_INFO loadInfo;
		loadInfo.Width = imageInfo.Width;
		loadInfo.Height = imageInfo.Height;
		loadInfo.Depth = 0;
		loadInfo.FirstMipLevel = 0;
		loadInfo.MipLevels = (noMipLevels) ? 1 : D3DX10_DEFAULT;
		loadInfo.Usage = D3D10_USAGE_IMMUTABLE;
		loadInfo.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		loadInfo.CpuAccessFlags = 0;
		loadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		loadInfo.Filter = D3DX10_FILTER_TRIANGLE;
		loadInfo.MipFilter = D3DX10_FILTER_TRIANGLE;
		loadInfo.pSrcInfo = &imageInfo;
				
		if SUCCEEDED(D3DX10CreateTextureFromFile(
			&m_D3D,
			path.c_str(),
			&loadInfo,
			NULL,
			reinterpret_cast<ID3D10Resource **>(&pResource),
			NULL))
		{
			VERIFY(SUCCEEDED(m_D3D.CreateShaderResourceView(pResource, NULL, &pView)));
			return new Texture2D(pResource, pView);
		}
	}

	SetLastError(L"Can not create 32-bit texture from file: '" + path + L"'");
	return NULL;
}

Renderer::Texture2D *Renderer::CreateRenderTarget(unsigned int width, unsigned int height)
{
	D3D10_TEXTURE2D_DESC texDesc;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D10_USAGE_DEFAULT;
	texDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE | D3D10_BIND_RENDER_TARGET;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D10Texture2D *pResource;
	if (S_OK == m_D3D.CreateTexture2D(&texDesc, NULL, &pResource))
	{
		ID3D10ShaderResourceView *pSRV;
		VERIFY(S_OK == m_D3D.CreateShaderResourceView(pResource, NULL, &pSRV));

		ID3D10RenderTargetView *pRTV;
		VERIFY(S_OK == m_D3D.CreateRenderTargetView(pResource, NULL, &pRTV));
		
		return new Texture2D(pResource, pSRV, pRTV);
	}

	return NULL;
}

bool Renderer::CompileShader(const std::wstring &path, const std::string &profile, ID3D10Blob **ppBytecode)
{
	ID3D10Blob *pBytecode = NULL;
	ID3D10Blob *pErrorMessages = NULL;

	HRESULT hResult;
	if FAILED(hResult = D3DX10CompileFromFile(
		path.c_str(),
		NULL,
		NULL, // LPD3D10INCLUDE pInclude
		"main",
		profile.c_str(),
		D3D10_SHADER_OPTIMIZATION_LEVEL3 | D3D10_SHADER_WARNINGS_ARE_ERRORS,
		0,
		NULL,
		&pBytecode,
		&pErrorMessages,
		NULL))
	{
		if (NULL != pErrorMessages)
		{
			const std::string compilerOutput(reinterpret_cast<char *>(pErrorMessages->GetBufferPointer()));
			SetLastError(
				L"Can not compile shader: '" + path + L"'\r\n" +
				L"\r\n" + 
				std::wstring(compilerOutput.begin(), compilerOutput.end()));
			
			pErrorMessages->Release();
		}
		else
		{
			if (D3D10_ERROR_FILE_NOT_FOUND == hResult)			
				SetLastError(L"Can not find: '" + path + L"'");
			else
			{
				ASSERT(0);
				SetLastError(
					L"Can not compile shader (unspecified error): '" + path + L"'\r\n" +
					L"This is probably caused by an outdated DirectX distribution.");
			}
		}
		
		return false;
	}
	
	*ppBytecode = pBytecode;
	SAFE_RELEASE(pErrorMessages);

	return true;
}

ID3D10VertexShader *Renderer::CreateVertexShader(const BYTE *pBytecode, size_t size)
{
	ID3D10VertexShader *pShader;
	VERIFY(SUCCEEDED(m_D3D.CreateVertexShader(pBytecode, size, &pShader)));
	return pShader;
}

ID3D10PixelShader *Renderer::CreatePixelShader(const BYTE *pBytecode, size_t size)
{
	ID3D10PixelShader *pShader;
	VERIFY(SUCCEEDED(m_D3D.CreatePixelShader(pBytecode, size, &pShader)));
	return pShader;
}

ID3D10VertexShader *Renderer::CompileVertexShader(const std::wstring &path, ID3D10Blob **ppBytecode /* = NULL */)
{ 
	ID3D10Blob *pBytecode = NULL;
	if (!CompileShader(path, "vs_4_0", &pBytecode))
		return NULL;
	
	ID3D10VertexShader *pShader;
	VERIFY(SUCCEEDED(m_D3D.CreateVertexShader(pBytecode->GetBufferPointer(), pBytecode->GetBufferSize(), &pShader)));

	if (NULL == ppBytecode)
	{
		SAFE_RELEASE(pBytecode);
	}
	else
	{
		// bytecode may be needed by ID3D10Device::CreateInputLayout()	
		*ppBytecode = pBytecode; 
	}
	
	return pShader;
}

ID3D10PixelShader *Renderer::CompilePixelShader(const std::wstring &path)
{
	ID3D10Blob *pBytecode = NULL;
	if (!CompileShader(path, "ps_4_0", &pBytecode))
		return NULL;

	ID3D10PixelShader *pShader;
	VERIFY(SUCCEEDED(m_D3D.CreatePixelShader(pBytecode->GetBufferPointer(), pBytecode->GetBufferSize(), &pShader)));

	SAFE_RELEASE(pBytecode);
	
	return pShader;
}

void Renderer::BeginFrame(const D3DXCOLOR &clearColor)
{
	ASSERT(!m_isInFrame);
	m_isInFrame = true;

	// clear entire back buffer
	m_D3D.RSSetViewports(1, &m_fullVP);
	m_D3D.ClearRenderTargetView(m_pBackBufferRTV, clearColor);

	// set adj. viewport
	m_D3D.RSSetViewports(1, &m_adjVP);
}

void Renderer::EndFrame()
{
	ASSERT(m_isInFrame);
	m_isInFrame = false;

	FlushSprites();
}

inline const D3DXVECTOR3 Rotate(const D3DXVECTOR2 &position, const D3DXVECTOR2 &pivot, float angle)
{
	if (0.f == angle)
	{
		return D3DXVECTOR3(position.x, position.y, 1.f);
	}
	else
	{
		D3DXMATRIX mRotZ;
		D3DXMatrixRotationZ(&mRotZ, angle);
		D3DXVECTOR4 vOut;
		D3DXVec2Transform(&vOut, &D3DXVECTOR2(position.x-pivot.x, position.y-pivot.y), &mRotZ);
		return D3DXVECTOR3(vOut.x+pivot.x, vOut.y+pivot.y, 1.f);
	}
}

void Renderer::AddSprite(
	const Texture2D *pTexture, 
	BlendMode blendMode, 
	const D3DXCOLOR &color,
	const D3DXVECTOR2 &topLeft, 
	const D3DXVECTOR2 &size, 
	float sortZ,
	float rotateZ /* = 0.f */,
	ID3D10PixelShader *pPS /* = NULL */)
{
	ASSERT(m_isInFrame);
	ASSERT(blendMode < kNumBlendModes);
	ASSERT(m_sprites.size() < kMaxSprites);
	
	// first sprite?
	if (true == m_sprites.empty())
	{
		// yes: lock vertex buffer
		ASSERT(NULL == m_pSpriteVertices);
		VERIFY(SUCCEEDED(m_pSpriteVB->Map(D3D10_MAP_WRITE_DISCARD, 0, reinterpret_cast<void **>(&m_pSpriteVertices))));
	}

	// hack: transform from top-left aligned 1920*1080 to homogenous space
	const D3DXVECTOR2 adjTopLeft = D3DXVECTOR2(-1.f + topLeft.x/1920.f*2.f, 1.f - topLeft.y/1080.f*2.f);
	const D3DXVECTOR2 adjSize = D3DXVECTOR2(size.x/1920.f*2.f, -size.y/1080.f*2.f);
	
	const D3DXVECTOR2 bottomRight = adjTopLeft + adjSize;
	const D3DXVECTOR2 quadPivot(adjTopLeft.x + adjSize.x*0.5f, adjTopLeft.y + adjSize.y*0.5f);

	const uint32_t ARGB = color;

	// triangle 1: bottom right
	m_pSpriteVertices->position = Rotate(D3DXVECTOR2(bottomRight.x, bottomRight.y), quadPivot, rotateZ);
	m_pSpriteVertices->ARGB = ARGB;
	m_pSpriteVertices->UV = D3DXVECTOR2(1.f, 1.f);
	++m_pSpriteVertices;

	// triangle 1: bottom left
	m_pSpriteVertices->position = Rotate(D3DXVECTOR2(adjTopLeft.x, bottomRight.y), quadPivot, rotateZ);
	m_pSpriteVertices->ARGB = ARGB;
	m_pSpriteVertices->UV = D3DXVECTOR2(0.f, 1.f);
	++m_pSpriteVertices;

	// triangle 1: top left
	m_pSpriteVertices->position = Rotate(D3DXVECTOR2(adjTopLeft.x, adjTopLeft.y), quadPivot, rotateZ);
	m_pSpriteVertices->ARGB = ARGB;
	m_pSpriteVertices->UV = D3DXVECTOR2(0.f, 0.f);
	++m_pSpriteVertices;

	// triangle 2: bottom right
	m_pSpriteVertices->position = Rotate(D3DXVECTOR2(bottomRight.x, bottomRight.y), quadPivot, rotateZ);
	m_pSpriteVertices->ARGB = ARGB;
	m_pSpriteVertices->UV = D3DXVECTOR2(1.f, 1.f);
	++m_pSpriteVertices;

	// triangle 2: top left
	m_pSpriteVertices->position = Rotate(D3DXVECTOR2(adjTopLeft.x, adjTopLeft.y), quadPivot, rotateZ);
	m_pSpriteVertices->ARGB = ARGB;
	m_pSpriteVertices->UV = D3DXVECTOR2(0.f, 0.f);
	++m_pSpriteVertices;

	// triangle 2: top right
	m_pSpriteVertices->position = Rotate(D3DXVECTOR2(bottomRight.x, adjTopLeft.y), quadPivot, rotateZ);
	m_pSpriteVertices->ARGB = ARGB;
	m_pSpriteVertices->UV = D3DXVECTOR2(1.f, 0.f);
	++m_pSpriteVertices;

	// add to list
	Sprite sprite;
	sprite.pPS = (NULL == pPS) ? m_pSpritePS : pPS;
	sprite.pTextureSRV = (NULL == pTexture) ? m_pWhiteTex->m_pSRV : pTexture->m_pSRV;
	sprite.blendMode = blendMode;
	sprite.sortZ = sortZ;
	sprite.vertexOffs = m_sprites.size()*6;
	m_sprites.push_back(sprite);
}

void Renderer::DrawRibbon(float sortZ, float alpha, float time)
{
	// hack: upload time right here and now (only rendering 1 ribbon anyway)
	float *pFloat4;
	m_pRibbonConst->Map(D3D10_MAP_WRITE_DISCARD, 0, reinterpret_cast<void **>(&pFloat4));
	pFloat4[0] = time;
	pFloat4[1] = 0.f;
	pFloat4[2] = 0.f;
	pFloat4[3] = 0.f;
	m_pRibbonConst->Unmap();
	m_D3D.PSSetConstantBuffers(0, 1, &m_pRibbonConst);

	AddSprite(
		NULL,
		kSubtractive,
		D3DXCOLOR(1.f, 1.f, 1.f, alpha),
		D3DXVECTOR2(0.f, 0.f),
		D3DXVECTOR2(1920.f, 1080.f),
		sortZ,
		0.f,
		m_pRibbonPS);
}

void Renderer::FlushSprites()
{
	if (false == m_sprites.empty())
	{
		// sort
		m_sprites.sort();

		// unlock vertex buffer
		ASSERT(NULL != m_pSpriteVertices);
		m_pSpriteVB->Unmap();
		m_pSpriteVertices = NULL;

		// set vertex stream
		const UINT stride = sizeof(SpriteVertex);
		const UINT offset = 0;
		m_D3D.IASetVertexBuffers(0, 1, &m_pSpriteVB, &stride, &offset);
		m_D3D.IASetInputLayout(m_pSpriteVertexLayout);
		m_D3D.IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// set shaders
		m_D3D.VSSetShader(m_pSpriteVS);
		m_D3D.GSSetShader(NULL);
//		m_D3D.PSSetShader(m_pSpritePS);

		// set sampler
		m_D3D.PSSetSamplers(0, 1, &m_pDefSamplerState);

		// draw
		ID3D10PixelShader *pCurPS = NULL;
		for (std::list<Sprite>::iterator iSprite = m_sprites.begin(); iSprite != m_sprites.end(); ++iSprite)
		{
			// set pixel shader (if necessary)
			if (pCurPS != iSprite->pPS)
			{
				pCurPS = iSprite->pPS;
				m_D3D.PSSetShader(pCurPS);
			}

			// set texture & blend state
			m_D3D.PSSetShaderResources(0, 1, &iSprite->pTextureSRV);
			m_D3D.OMSetBlendState(m_pBlendStates[iSprite->blendMode], NULL, 0xffffffff);

			// draw
			m_D3D.Draw(6, iSprite->vertexOffs);
		}
	}

	m_sprites.clear();
}
