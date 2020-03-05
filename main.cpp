#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <d3d8.h>
#pragma comment (lib, "d3d8.lib")
#include <d3dx8.h>
#pragma comment (lib, "d3dx8.lib")
#include <MMSystem.h>
#pragma comment (lib, "WinMM.Lib")

#define APPNAME  "Water Effect "
char	AppTitle[96];
#define _RELEASE_(p) { if(p) { (p)->Release(); (p)=NULL; };};
#define _DELETE_(p)  { if(p) { delete (p);     (p)=NULL; };};

LPDIRECT3D8             p_d3d          = NULL;
LPDIRECT3DDEVICE8       p_d3d_Device   = NULL;
LPDIRECT3DVERTEXBUFFER8 p_VertexBuffer = NULL;
LPDIRECT3DINDEXBUFFER8  p_IndexBuffer  = NULL;
LPDIRECT3DTEXTURE8		p_Texture_001  = NULL;

D3DPRESENT_PARAMETERS	d3dpp;
D3DDISPLAYMODE			d3ddm;

D3DLIGHT8 light;
D3DTEXTUREFILTERTYPE CurrentFilter;
D3DFILLMODE			 CurrentFillMd;
DWORD				 vertex_shader;

D3DXMATRIX matWorld;
D3DXMATRIX matProj;

D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );

#define D3DFVF_MYVERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)
struct MYVERTEX {D3DXVECTOR3 pos,nrm; DWORD diffuse; D3DXVECTOR2 tex;};

HWND hWnd;
HINSTANCE hTInst,hPInst;
#define tconst 1024.0f;
float tshift;
DWORD timeperiod;
int CmdShow,cnt,cntm=1;
bool fPlayMode=true;
bool fWiteMode=false;
bool fAlfaBlnd=false;
bool isFullScreen=false;
#define G_N  254
#define G_VN ((G_N+1)*(G_N+1))
#define G_IN (G_N*G_N*6)
float   G_AB=0.5f;
#define vis 0.00025f

struct surf{
	float H[G_N+1][G_N+1];
};

BYTE  tmpf;
DWORD tp1,tp2,tpm=16;
surf S1,S2;
surf *p=&S1,*n=&S2;

void DestroyDirect3D8 (void){
	_RELEASE_ (p_Texture_001);
	_RELEASE_ (p_IndexBuffer);
	_RELEASE_ (p_VertexBuffer);
	_RELEASE_ (p_d3d_Device);
	_RELEASE_ (p_d3d);
};

HRESULT InitVertexShaders(void){
	char d3derrmsg[64];
    LPD3DXBUFFER pShaderCode;
    DWORD dwShaderDecl[] = {
        D3DVSD_STREAM(0),
        D3DVSD_REG(D3DVSDE_POSITION,D3DVSDT_FLOAT3),
		D3DVSD_REG(D3DVSDE_NORMAL, D3DVSDT_FLOAT3),
        D3DVSD_REG(D3DVSDE_DIFFUSE,D3DVSDT_D3DCOLOR),
		D3DVSD_REG(D3DVSDE_TEXCOORD0,D3DVSDT_FLOAT2),
        D3DVSD_END()
    };
    HRESULT hr;
	try {
		hr = D3DXAssembleShaderFromFile("water.vsh", 0, NULL, &pShaderCode, NULL);
	}
	catch (...){
		MessageBox(NULL, "An exception has been raised while compiling a vertex shader program.", "ERROR", MB_ICONERROR);
		return E_FAIL;
	}
	if (hr){
		sprintf(d3derrmsg, "While compiling VS returned an error code: %dx", hr);
		MessageBox(NULL, d3derrmsg, "ERROR", MB_ICONERROR);
		return E_FAIL;
	}else{
		hr = p_d3d_Device->CreateVertexShader(dwShaderDecl, (DWORD*)pShaderCode->GetBufferPointer(),&vertex_shader,0);
	};
    pShaderCode->Release();
	return hr;
};

HRESULT TransGeometry(void){
	MYVERTEX* pVertices;
	if(WORD(rand()%64)==0){
		int ii=rand()%(G_N-27),jj=rand()%(G_N-27);
		tmpf++;
		for(int i=-7;i<8;i++){
			for(int j=-7;j<8;j++){
				float v=24.0f-i*i-j*j;
				if(v<0.0f) v=0.0f;
				if (tmpf&1) n->H[i+ii+3][j+jj+3]-=v*0.025f; else n->H[i+ii+3][j+jj+3]+=v*0.025f;
			};
		};
	};
	if(FAILED(p_VertexBuffer->Lock (0,G_VN*sizeof(MYVERTEX),(BYTE**)&pVertices,	0))) return E_FAIL;
	D3DXVECTOR3 nrm;
	for(WORD i=0;i<(G_N+1);i++){
		for(WORD j=0;j<(G_N+1);j++){
			pVertices[i+j*(G_N+1)].pos.y=n->H[i][j];
			pVertices[i+j*(G_N+1)].nrm=D3DXVECTOR3(
				float(n->H[WORD(i-1)%(G_N+1)][j] - n->H[i][j]),
				1.0f,
				float(n->H[WORD(i-1)%(G_N+1)][j] - n->H[i][j]));

			float laplas = (n->H[WORD(i-1)%(G_N+1)][j] +
							n->H[WORD(i+1)%(G_N+1)][j] +
							n->H[i][WORD(j+1)%(G_N+1)] +
							n->H[i][WORD(j-1)%(G_N+1)])*0.25f - n->H[i][j];

			p->H[i][j] = ((2.0f-vis)*n->H[i][j] - p->H[i][j]*(1.0f-vis) + laplas);
		}
	}
	p_VertexBuffer->Unlock();
	surf *sw=p;p=n;n=sw;
};

HRESULT InitGeometry(){
	if(FAILED(p_d3d_Device->CreateVertexBuffer (
		G_VN*sizeof(MYVERTEX),
		0,
		D3DFVF_MYVERTEX,
		D3DPOOL_MANAGED,
		&p_VertexBuffer))) return E_FAIL;
	MYVERTEX* pVertices;
	if(FAILED(p_VertexBuffer->Lock (
		0,
		G_VN*sizeof(MYVERTEX),
		(BYTE**)&pVertices,
		0))) return E_FAIL;
	for (int i=0;i<G_N+1;i++){
		for (int j=0;j<G_N+1;j++){
			pVertices[i+j*(G_N+1)].pos=D3DXVECTOR3((float)(i-G_N/2)*G_AB,0.0f,(float)(j-G_N/2)*G_AB);
			pVertices[i+j*(G_N+1)].nrm=D3DXVECTOR3(0.0f,1.0f,0.0f);
			pVertices[i+j*(G_N+1)].tex=D3DXVECTOR2((float)i/(G_N+1),(float)j/(G_N+1));
			DWORD diff=0x00804040;
			pVertices[i+j*(G_N+1)].diffuse=diff;
		};
	};
	p_VertexBuffer->Unlock();
	if(FAILED(p_d3d_Device->CreateIndexBuffer (
		G_IN*sizeof(WORD),
		0,
		D3DFMT_INDEX16,
		D3DPOOL_MANAGED,
		&p_IndexBuffer))) return E_FAIL;
	WORD* pVerticesI;
	if(FAILED(p_IndexBuffer->Lock (
		0,
		G_IN*sizeof(WORD),
		(BYTE**)&pVerticesI,
		0))) return E_FAIL;
	for(int i=0, ii=0; ii<G_N; ii++ ){
		for(int j=0; j<G_N; j++){
			pVerticesI[i++] = (j+0) + (ii+1)*(G_N+1);
			pVerticesI[i++] = (j+1) + (ii+0)*(G_N+1);
			pVerticesI[i++] = (j+0) + (ii+0)*(G_N+1);
			pVerticesI[i++] = (j+0) + (ii+1)*(G_N+1);
			pVerticesI[i++] = (j+1) + (ii+1)*(G_N+1);
			pVerticesI[i++] = (j+1) + (ii+0)*(G_N+1);
		};
	};
	p_IndexBuffer->Unlock();
	return  S_OK;
};

void SetD3DMode(){
	p_d3d_Device->LightEnable (0, true);
	p_d3d_Device->SetRenderState(D3DRS_LIGHTING, true);
	p_d3d_Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	p_d3d_Device->SetRenderState(D3DRS_ZENABLE, TRUE);
	p_d3d_Device->SetRenderState(D3DRS_FILLMODE, CurrentFillMd);
	p_d3d_Device->SetRenderState (D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	p_d3d_Device->SetRenderState (D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	p_d3d_Device->SetRenderState (D3DRS_LOCALVIEWER, true);
	p_d3d_Device->SetRenderState (D3DRS_NORMALIZENORMALS,true);
    D3DXMatrixPerspectiveFovLH(
		&matProj,
		D3DX_PI/4,
		1.3f,
		1.0f,
		1000.0f);
    p_d3d_Device->SetTransform(D3DTS_PROJECTION, &matProj);
};

HRESULT SetUpD3D(void){
	if(NULL == (p_d3d = Direct3DCreate8(D3D_SDK_VERSION))) return E_FAIL;
	if(FAILED(p_d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm))) return E_FAIL;
	ZeroMemory (&d3dpp, sizeof(d3dpp));
	d3dpp.BackBufferFormat = d3ddm.Format;
	d3dpp.Windowed = true;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	if(FAILED(p_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &p_d3d_Device)))
		if(FAILED(p_d3d->CreateDevice(
			D3DADAPTER_DEFAULT,	D3DDEVTYPE_HAL,	hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&d3dpp,	&p_d3d_Device)))
			if(FAILED(p_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &p_d3d_Device))) return E_FAIL;

	ZeroMemory (&light, sizeof(D3DLIGHT8));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse.r  = 1.0f;
	light.Diffuse.g  = 1.0f;
	light.Diffuse.b  = 1.0f;
	light.Range = 1000.0f;
	D3DXMatrixIdentity(&matWorld);
    D3DXMatrixRotationY(&matWorld, 0);
    p_d3d_Device->SetTransform(D3DTS_WORLD, &matWorld);
	SetD3DMode();
	timeperiod=timeGetTime();
	while(timeperiod==timeGetTime());
	timeperiod=timeGetTime()-timeperiod;
	if (timeperiod>1024) timeperiod=-timeperiod;
	timeBeginPeriod(4);
	tshift=timeGetTime()/tconst;
	tshift-=3.0f*3.1415f;
	return S_OK;
}

VOID PlayWorld(){
	float t1=(DWORD)timeGetTime()/tconst;
	t1-=tshift;
	float t2=t1/2.0f;
	float t3=t1/3.0f;
	D3DXMATRIX  matView;
	D3DXVECTOR3 vEyePt( sin(t2)*(cos(t3)+1.1f)*80.0f, (sin(t2)/4+0.5f)*50.0f, cos(t2)*(cos(t3)+1.1f)*80);
	D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
	p_d3d_Device->SetTransform(D3DTS_VIEW, &matView);
	cnt=(++cnt)%cntm;
	D3DXVECTOR3 vecDir;
	if(cnt) {
		vecDir = D3DXVECTOR3(0.0f, -1.0f, -1.0f);
	}else{
		vecDir = D3DXVECTOR3(-cosf(t1),-2, sinf(t1));
	};
	D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &vecDir);
	p_d3d_Device->SetLight (0, &light);
	D3DXMATRIX mat;
	D3DXMatrixTranspose(&mat,&matWorld);
	p_d3d_Device->SetVertexShaderConstant(0,&mat,4);
	D3DXMatrixTranspose(&mat,&matView);
	p_d3d_Device->SetVertexShaderConstant(4,&mat,4);
	D3DXMatrixTranspose(&mat,&matProj);
	p_d3d_Device->SetVertexShaderConstant(8,&mat,4);
	p_d3d_Device->SetVertexShaderConstant( 12, &light.Direction, 1 );
	if (fPlayMode) {
		tp2=timeGetTime();
		if (tp2-tp1>tpm){
			tp1=tp2;
			TransGeometry();
		};
	};
};

void RenderScreen (void){
	HRESULT hr=p_d3d_Device->TestCooperativeLevel();
	if(hr==D3DERR_DEVICELOST) {
		Sleep( 50 );
		return;
	}
	if(hr==D3DERR_DEVICENOTRESET) {
		p_d3d_Device->Reset(&d3dpp);
		SetD3DMode();
	};
	if(!fWiteMode) {
		p_d3d_Device->Clear (0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB (0, 0, 0), 1.0f, 0);
	}else{
		p_d3d_Device->Clear (0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB (255, 255, 255), 1.0f, 0);
	}
	if (fAlfaBlnd) {
		p_d3d_Device->SetRenderState (D3DRS_ALPHABLENDENABLE, true);
	}else{
		p_d3d_Device->SetRenderState (D3DRS_ALPHABLENDENABLE, false);
	};
	D3DMATERIAL8 mtrl1;
	ZeroMemory (&mtrl1, sizeof(D3DMATERIAL8));
	p_d3d_Device->BeginScene ();
	PlayWorld();
	p_d3d_Device->SetStreamSource (0, p_VertexBuffer, sizeof(MYVERTEX));
	mtrl1.Diffuse.r = mtrl1.Ambient.r = 0.0f;
	mtrl1.Diffuse.g = mtrl1.Ambient.g = 0.0f;
	mtrl1.Diffuse.b = mtrl1.Ambient.b = 1.0f;
	mtrl1.Diffuse.a = mtrl1.Ambient.a = 1.0f;
	p_d3d_Device->SetMaterial (&mtrl1);
	p_d3d_Device->SetTexture(0, p_Texture_001);
	p_d3d_Device->SetTextureStageState(0, D3DTSS_MAGFILTER, CurrentFilter);
	p_d3d_Device->SetTextureStageState(0, D3DTSS_MINFILTER, CurrentFilter);
	p_d3d_Device->SetTextureStageState(0, D3DTSS_MIPFILTER, CurrentFilter);
	p_d3d_Device->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	p_d3d_Device->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	p_d3d_Device->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SUBTRACT);
	p_d3d_Device->SetTextureStageState (0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
	p_d3d_Device->SetTextureStageState (0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
	p_d3d_Device->SetVertexShader (D3DFVF_MYVERTEX);
	p_d3d_Device->SetIndices (p_IndexBuffer, 0);
	p_d3d_Device->DrawIndexedPrimitive (D3DPT_TRIANGLELIST, 0, G_IN, 0, G_IN/3);
	p_d3d_Device->EndScene();
	p_d3d_Device->Present (NULL, NULL, NULL, NULL);
};

void SetTitle(){
	strcpy(AppTitle,APPNAME);
	switch (CurrentFilter){
	case D3DTEXF_NONE:
		strcat(AppTitle,"(Filter=D3DTEXF_NONE ");
		break;
	case D3DTEXF_POINT:
		strcat(AppTitle,"(Filter=D3DTEXF_POINT ");
		break;
	case D3DTEXF_LINEAR:
		strcat(AppTitle,"(Filter=D3DTEXF_LINEAR ");
		break;
	case D3DTEXF_ANISOTROPIC:
		strcat(AppTitle,"(Filter=D3DTEXF_ANISOTROPIC ");
		break;
	};
	switch (CurrentFillMd){
	case D3DFILL_POINT:
		strcat(AppTitle,"FillMode=D3DFILL_POINT)");
		break;
	case D3DFILL_WIREFRAME:
		strcat(AppTitle,"FillMode=D3DFILL_WIREFRAME)");
		break;
	case D3DFILL_SOLID:
		strcat(AppTitle,"FillMode=D3DFILL_SOLID)");
		break;
	};
	strcat(AppTitle," Copyright (c) Grushetsky V.A.");
};

LRESULT CALLBACK WindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	BOOL modechangefl;
	switch (message){
	case WM_KEYDOWN:{
		int VK = (int)wParam;
		if (VK==VK_HOME) tpm=tpm<<1;
		if (VK==VK_END) if (!tpm) tpm++; else tpm=tpm>>1;
		if (VK==VK_NEXT) cntm++; else
		if (VK==VK_PRIOR){
			if (cntm>1) cntm--;
		}else
		if (VK==VK_RETURN){
			d3dpp.Windowed=isFullScreen;
			isFullScreen=!isFullScreen;
			d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
			d3dpp.BackBufferWidth = d3ddm.Width;
			d3dpp.BackBufferHeight = d3ddm.Height;
			d3dpp.FullScreen_RefreshRateInHz = d3ddm.RefreshRate;
			d3dpp.BackBufferFormat = d3ddm.Format;
			d3dpp.EnableAutoDepthStencil = true;
			d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
			if (isFullScreen){
				SetWindowLong (hWnd, GWL_STYLE, WS_POPUP);
				SetWindowPos (hWnd, 0, 0, 0, d3ddm.Width, d3ddm.Height, 0);
				SetCursor (NULL);
			}else{
				SetWindowLong (hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_MAXIMIZEBOX);
				SetWindowPos (hWnd, 0, 0, 0, 640, 480, 0);
				SetCursor (LoadCursor (NULL, IDC_ARROW));
			};
			p_d3d_Device->Reset(&d3dpp);
			SetD3DMode();
		};
		modechangefl=true;
		if (VK==VK_F8)  {fPlayMode=!fPlayMode;} else
		if (VK==VK_F9)  {fWiteMode=!fWiteMode;} else
		if (VK==VK_F11) {fAlfaBlnd=!fAlfaBlnd;} else
		switch(VK){
		case VK_F1:
			CurrentFilter = D3DTEXF_NONE;
			break;
		case VK_F2:
			CurrentFilter = D3DTEXF_POINT;
			break;
		case VK_F3:
			CurrentFilter = D3DTEXF_LINEAR;
			break;
		case VK_F4:
			CurrentFilter = D3DTEXF_ANISOTROPIC;
			break;
		case VK_F5:
			CurrentFillMd = D3DFILL_POINT;
			break;
		case VK_F6:
			CurrentFillMd = D3DFILL_WIREFRAME;
			break;
		case VK_F7:
			CurrentFillMd = D3DFILL_SOLID;
			break;
		default:
			modechangefl=false;
		};
		if (modechangefl){
			SetTitle();
			SetWindowText(hWnd, AppTitle);
			p_d3d_Device->SetRenderState (D3DRS_FILLMODE, CurrentFillMd);
		};
		break;
	};
	case WM_DESTROY:
		PostQuitMessage (0);
		break;
	};
	return DefWindowProc(hWnd, message, wParam, lParam);
};

bool WindowInit (HINSTANCE hThisInst, int nCmdShow){
	WNDCLASS		    wcl;
	wcl.hInstance		= hThisInst;
	wcl.lpszClassName	= APPNAME;
	wcl.lpfnWndProc		= WindowProc;
	wcl.style			= 0;
	wcl.hIcon			= LoadIcon (hThisInst, IDC_ICON);
	wcl.hCursor			= LoadCursor (hThisInst, IDC_ARROW);
	wcl.lpszMenuName	= NULL;
	wcl.cbClsExtra		= 0;
	wcl.cbWndExtra		= 0;
	wcl.hbrBackground	= (HBRUSH) GetStockObject (BLACK_BRUSH);
	RegisterClass (&wcl);
	hWnd = CreateWindow(
		APPNAME,
		AppTitle,
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_MAXIMIZEBOX,

		0, 0,
		640,
		480,
		NULL,
		NULL,
		hThisInst,
		NULL);
	if(!hWnd) return false;
	return true;
};

int APIENTRY WinMain (HINSTANCE hThisInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow){
	MSG msg;
	HRESULT d3derrcode;
	char d3derrmsg[64];
	hTInst=hThisInst;
	hPInst=hPrevInst;
	CmdShow=nCmdShow;
	CurrentFilter = D3DTEXF_NONE;
	CurrentFillMd = D3DFILL_SOLID;
	SetTitle();
	if(!WindowInit (hThisInst, nCmdShow)) return 1;
	d3derrcode=SetUpD3D();
	if(!d3derrcode)
		d3derrcode=InitGeometry();
	if (d3derrcode){
		sprintf(d3derrmsg, "Initialization procedure return an error code: %dx", d3derrcode);
		MessageBox(NULL, d3derrmsg, "ERROR", MB_ICONERROR);
		return 2;
	};
	if (InitVertexShaders()) return 3;
	if(FAILED(D3DXCreateTextureFromFile(p_d3d_Device, "005.png", &p_Texture_001))){
		MessageBox(NULL, "Image 005.png not found", "ERROR", MB_ICONERROR);
		return 4;
	}
	ShowWindow (hWnd, nCmdShow);
	UpdateWindow (hWnd);
	SetCursor (LoadCursor (NULL, IDC_ARROW));
	while (1)	{
		if(PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)){
			if(!GetMessage (&msg, NULL, 0, 0)) break;
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}else RenderScreen ();
	};
	timeEndPeriod(timeperiod);
	DestroyDirect3D8();
	return 0;
};

