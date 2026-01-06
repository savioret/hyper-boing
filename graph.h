#ifndef GRAPH_H_
#define GRAPH_H_


#define RENDERMODE_NORMAL 1
#define RENDERMODE_EXCLUSIVE 2

#define RES_X 640
#define RES_Y 480

class GRAPH;
class BMNUMFONT;

/********************************************************
  clase SPRITE

  Es el objeto Sprite, como su nombre indica. Este contiene
  informacion sobre el bitmap, sus dimensiones para gestionar
  colisiones, y el desplazamiento "relativo".
  xoff yoff > Se utilizan para representar secuencias de animacion
    por ejemplo, si un personaje esta andando, al caminar todos
    sus fotogramas deben quedar alineados con su cuerpo, de lo contrario
    parecerá que este haciendo el baile del sambito. Xoff e Yoff
    son valores que ayudan a ajustar y corregir esto.
********************************************************/
class SPRITE
{
public:
    SDL_Texture* bmp;
    int sx, sy;
    int xoff, yoff; //desplazamiento relativo
    GRAPH * graph;

    void Init(GRAPH *gr, char file[], int offx=0, int offy=0);
    void Release ( void );
};



/********************************************************
  clase GRAPH

  Es la clase que realiza las funciones "primitivas" de pintado.
  De la misma forma, inicia el modo grafico, y lo restaura.
  Tambien carga imagenes BMP.
  Todo ello usando DirectX.
********************************************************/
class GRAPH
{
public:	
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* backBuffer;
    int mode;

    int Init ( const char* title, int _mode );
    int InitNormal ( const char* title );
    int InitEx ( const char* title );
    void Release ();


    void Draw ( SPRITE* spr, int x, int y );
    void DrawScaled ( SPRITE* spr, int x, int y, int w, int h ); // NEW
    void Draw ( SDL_Texture* texture, RECT* srcRect, int x, int y );
    void DrawClipped ( SDL_Texture* texture, RECT* srcRect, int x, int y );
    void DrawClipped ( SPRITE* spr, int x, int y );
    void Draw ( BMNUMFONT* font, int num, int x, int y );
    void Draw ( BMNUMFONT* font, char cad[], int x, int y );
    void DrawClipped ( BMNUMFONT* font, char cad[], int x, int y );

    void Flip();

    void Text(const char texto[], int x, int y);
    void Rectangle(int a, int b, int c, int d);
    //void MoveTo(int x, int y);
    //void LineTo(int x, int y);

    HRESULT CopyBitmap ( SDL_Texture* texture, SDL_Surface* surface, int x, int y, int dx, int dy );
    void LoadBitmap ( SPRITE* spr, const char* szBitmap );
    Uint32 ColorMatch ( SDL_Surface* surface, Uint32 rgb );
    HRESULT SetColorKey ( SDL_Texture* texture, Uint32 rgb );
    //HRESULT SetColorKey(IDirectDrawSurface7 * pdds, COLORREF rgb);
    //DWORD ColorMatch(IDirectDrawSurface7 * pdds, COLORREF rgb);
};

#endif