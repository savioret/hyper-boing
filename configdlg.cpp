//#include <windows.h>
//#include "resource.h"
#include "pang.h"



#define NUMKEYS 98

extern HINSTANCE hInst;
extern HWND hWndMain;
extern HWND hConfigDialog;
extern PAPP *screen;

void SelectComboText(HWND hwnd, int item, char *text);



/********************************************************
  clase KEYTABLE

  Esta clase contiene informacion con todas las teclas disponibles
  y configurables para el juego, con su equivalente en VIRTUAL KEYS
  y su equivalente en TEXTO.

  La idea era que se pudieran configurar las teclas ya sea
  seleccionandolas de un ComboBox, o bien mediante la pulsacion
  de una tecla del teclado, pero por lo visto los Dialogos no
  reciben mensajes de WM_CHAR o WM_KEYDOWN.
********************************************************/
class KEYTABLE
{
public:
	SDL_Scancode dik[SDL_NUM_SCANCODES];
	int vk[NUMKEYS];
	char kname[NUMKEYS][64];

	int GetDIKeyText( SDL_Scancode param, char *text);
	const char * GetDIKeyText( SDL_Scancode param);
	int GetVrtKeyText(int param, char *text);
	char * GetVrtKeyText(int param);
	char * GetText(int index);

	SDL_Scancode GetDIK(char *text);
	
	SDL_Scancode ToDI(int vkey);
	int ToVK( SDL_Scancode dikey);

	KEYTABLE();
};


BOOL CALLBACK ConfigDlgMsgProc(HWND hWndDlg, WORD Message, WORD wParam, LONG lParam)
{
	int res;
	KEYTABLE keytbl;
	char keys1[3][32], keys2[3][32];	

 switch(Message)
   {
    case WM_INITDIALOG:
		int i;// , index;

        res = keytbl.GetDIKeyText(gameinf.keys[PLAYER1].left, keys1[0]);	
		res = keytbl.GetDIKeyText(gameinf.keys[PLAYER1].right, keys1[1]);
		res = keytbl.GetDIKeyText(gameinf.keys[PLAYER1].shoot, keys1[2]);
		res = keytbl.GetDIKeyText(gameinf.keys[PLAYER2].left, keys2[0]);	
		res = keytbl.GetDIKeyText(gameinf.keys[PLAYER2].right, keys2[1]);
		res = keytbl.GetDIKeyText(gameinf.keys[PLAYER2].shoot, keys2[2]);
		for(i=0; i<NUMKEYS; i++)
		{
			SendDlgItemMessage(hWndDlg, IDC_KEYCONF1_LEFT, CB_ADDSTRING,
                                 0,  (LPARAM) (LPCTSTR) keytbl.GetText(i)) ;
			SendDlgItemMessage(hWndDlg, IDC_KEYCONF1_RIGHT, CB_ADDSTRING,
                                 0,  (LPARAM) (LPCTSTR) keytbl.GetText(i)) ;
			SendDlgItemMessage(hWndDlg, IDC_KEYCONF1_SHOOT, CB_ADDSTRING,
                                 0,  (LPARAM) (LPCTSTR) keytbl.GetText(i)) ;
			SendDlgItemMessage(hWndDlg, IDC_KEYCONF2_LEFT, CB_ADDSTRING,
                                 0,  (LPARAM) (LPCTSTR) keytbl.GetText(i)) ;
			SendDlgItemMessage(hWndDlg, IDC_KEYCONF2_RIGHT, CB_ADDSTRING,
                                 0,  (LPARAM) (LPCTSTR) keytbl.GetText(i)) ;
			SendDlgItemMessage(hWndDlg, IDC_KEYCONF2_SHOOT, CB_ADDSTRING,
                                 0,  (LPARAM) (LPCTSTR) keytbl.GetText(i)) ;
		}

		SelectComboText(hWndDlg, IDC_KEYCONF1_LEFT, keys1[0]);
		SelectComboText(hWndDlg, IDC_KEYCONF1_RIGHT, keys1[1]);
		SelectComboText(hWndDlg, IDC_KEYCONF1_SHOOT, keys1[2]);
		SelectComboText(hWndDlg, IDC_KEYCONF2_LEFT, keys2[0]);
		SelectComboText(hWndDlg, IDC_KEYCONF2_RIGHT, keys2[1]);
		SelectComboText(hWndDlg, IDC_KEYCONF2_SHOOT, keys2[2]);
         break; /* Fin de WM_INITDIALOG */

    case WM_CLOSE:
         /* Cerrar el dialogo es lo mismo que cancelar  */
         PostMessage(hWndDlg, WM_COMMAND, IDCANCEL, 0L);
         break;

    case WM_COMMAND:
         switch(wParam)
           {
            case IDOK:
				 screen->SetPause(FALSE);
				 GetDlgItemText(hWndDlg, IDC_KEYCONF1_LEFT, keys1[0], 256);
				 GetDlgItemText(hWndDlg, IDC_KEYCONF1_RIGHT, keys1[1], 256);
				 GetDlgItemText(hWndDlg, IDC_KEYCONF1_SHOOT, keys1[2], 256);
				 gameinf.keys[PLAYER1].SetLeft(keytbl.GetDIK(keys1[0]));
				 gameinf.keys[PLAYER1].SetRight(keytbl.GetDIK(keys1[1]));
				 gameinf.keys[PLAYER1].SetShoot(keytbl.GetDIK(keys1[2]));
				 GetDlgItemText(hWndDlg, IDC_KEYCONF2_LEFT, keys2[0], 256);
				 GetDlgItemText(hWndDlg, IDC_KEYCONF2_RIGHT, keys2[1], 256);
				 GetDlgItemText(hWndDlg, IDC_KEYCONF2_SHOOT, keys2[2], 256);
				 gameinf.keys[PLAYER2].SetLeft(keytbl.GetDIK(keys2[0]));
				 gameinf.keys[PLAYER2].SetRight(keytbl.GetDIK(keys2[1]));
				 gameinf.keys[PLAYER2].SetShoot(keytbl.GetDIK(keys2[2]));				
				 
                 EndDialog(hWndDlg, TRUE);
                 break;

			case IDCANCEL:
				screen->SetPause(FALSE);
				EndDialog(hWndDlg, TRUE);
			break;
				
           }
         break;

    default:
        return FALSE;
   }
 return TRUE;
}


void SelectComboText(HWND hwnd, int item, char *text)
{
	int index;
	
	
	index=SendDlgItemMessage(hwnd, item, CB_FINDSTRINGEXACT, -1, (LPARAM)(LPCSTR) text);
	if ( index != LB_ERR )
		SendDlgItemMessage(hwnd, item, CB_SETCURSEL,
                                 index,  0) ;
		
}



/**************************************************************************/


/**********************************************************
	CONSTRUCTOR

  Inicia las tablas de equivalencias de teclas.
***********************************************************/


KEYTABLE::KEYTABLE()
{
	for ( int i = SDL_SCANCODE_A; i < SDL_SCANCODE_ENDCALL; ++i ) {
		SDL_Scancode scancode = static_cast<SDL_Scancode>(i);
		const char* name = SDL_GetScancodeName ( scancode );
		if ( name )
			strcpy ( kname[i], name );
	}
}	


/********************************************************
 devuelve el texto de un indice de la tabla de nombres
********************************************************/
char * KEYTABLE::GetText(int index)
{
	return kname[index];
}


/********************************************************
 Devuelve la tecla DirectInput correspondiente al nombre
 de la tecla
********************************************************/
SDL_Scancode KEYTABLE::GetDIK(char *text)
{
	int i;

	for(i=0;i< NUMKEYS;i++)
		if(!strcmp(text,kname[i]))
		{			
			return dik[i];
		}

	return SDL_SCANCODE_UNKNOWN;
}

/********************************************************
	Guarda en <text> el nombre de una tecla DirectInput
   Y devuelve el indice correspondiente en la tabla.
********************************************************/
int KEYTABLE::GetDIKeyText( SDL_Scancode param, char *text)
{
	//int i;
	const char* name = GetDIKeyText ( param );
	if ( name )
		strcpy ( text, name );

	return param; // ????
}

/********************************************************
 Dada una tecla DirectInput, devuelve su equivalente en texto
********************************************************/
const char * KEYTABLE::GetDIKeyText( SDL_Scancode scancode )
{
	const char* name = SDL_GetScancodeName ( (SDL_Scancode) scancode );
	return name;
}

/********************************************************
 Dada una tecla VirtualKey guarda en <text> su equivalente
 en texto, y devuelve el indice de la tabla
********************************************************/
int KEYTABLE::GetVrtKeyText(int param, char *text)
{
	int i;

	for(i=0;i< NUMKEYS;i++)
		if(param == vk[i])
		{
			strcpy(text, kname[i]);
			return i;
		}

	return -1;
}


/********************************************************
 Dada una tevla VirtualKey devuelve su equivalente en texto
********************************************************/
char * KEYTABLE::GetVrtKeyText(int param)
{
	int i;

	for(i=0;i< NUMKEYS;i++)
		if(param == vk[i])
		{
			return kname[i];
		}

	return NULL;
}

/********************************************************
 Devuelve el equivalente de una tecla VirtualKey en DirectInput
********************************************************/
SDL_Scancode KEYTABLE::ToDI(int vkey)
{
	int i;

	for(i=0;i< NUMKEYS;i++)
		if(vkey == vk[i])
			return dik[i];

	return SDL_SCANCODE_UNKNOWN;
}

/********************************************************
 Devuelve el equivalente de una tecla DirectInput en VirtualKey
********************************************************/
int KEYTABLE::ToVK( SDL_Scancode dikey)
{
int i;

	for(i=0;i< NUMKEYS;i++)
		if(dikey == dik[i])
			return vk[i];

	return 0;
}

