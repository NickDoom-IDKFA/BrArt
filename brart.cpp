//Yet another "one-evening software" :)
//(C) NickDoom, GNU GENERAL PUBLIC LICENSE Ver. 3

#include <windows.h>
#include <fstream.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>

#pragma pack(push)
#pragma pack(0)
struct BMP_HEADER
{
	short Id;
	long FSize, Ver, DataOffset, Unused, Width, Height;
	short WTF, BPP;
	long Compr; //0 for uncompressed, 1,2 for RLE
	long DataSize,   XRes, YRes, PalSize, Unused2;
//	char Palette[256][4];
	char Palette[2][4];
};
#pragma pack(pop)

struct {
	int SX, SY;
	char BBCode[3][32];
	int MaxDiff;
	int Bgnd[3], Fgnd[3];
} Config = {1, 1, "[color=#XXXXXX]", "[/color]", "\n", 16, 255, 255, 255, 0, 0, 0};
//	char BBCode[3][32]={"<font color=#XXXXXX>", "</font>", "<br>"};

char ConstBraille=0xe2;
#define SqPifa(x,y,z) ( (x)*(x) + (y)*(y) + (z)*(z) )

int MonoPixel (char *Bitmap1bit, int x, int y, int W, int W32, int H, int Inv)
{
	if (x<W && y<H)
	 if (Inv)	return   Bitmap1bit[(H-(y)-1)*W32*4+(x)/8]&(128>>((x)%8))  ;
	 else		return !(Bitmap1bit[(H-(y)-1)*W32*4+(x)/8]&(128>>((x)%8)) );

	return Inv;
}

void Mono (fstream *InFile, int Offset, int X, int Y, int Inv, char *OutFile)
{
	char *file;
	unsigned short Word;
	int x, y, X32, CutTail=0;
	fstream f;

	X32=(X+31)/32;
	file=(char*)malloc(X32*Y*4);
	InFile->seekg(Offset);
	InFile->read (file,X32*Y*4);
	InFile->close();

	f.open (OutFile,ios::out|ios::binary);
	for (y=0;y<Y;y+=4+Config.SY,f.seekp(CutTail),f<<endl,CutTail=f.tellp())
	 for (x=0;x<X;x+=2+Config.SX)
	 {
		Word=0x80A0;
		if (MonoPixel(file,x+0,y+0,X,X32,Y,Inv)) Word|=0x100;
		if (MonoPixel(file,x+0,y+1,X,X32,Y,Inv)) Word|=0x200;
		if (MonoPixel(file,x+0,y+2,X,X32,Y,Inv)) Word|=0x400;
		if (MonoPixel(file,x+1,y+0,X,X32,Y,Inv)) Word|=0x800;
		if (MonoPixel(file,x+1,y+1,X,X32,Y,Inv)) Word|=0x1000;
		if (MonoPixel(file,x+1,y+2,X,X32,Y,Inv)) Word|=0x2000;
		if (MonoPixel(file,x+0,y+3,X,X32,Y,Inv)) Word|=0x1;
		if (MonoPixel(file,x+1,y+3,X,X32,Y,Inv)) Word|=0x2;
		f.write (&ConstBraille,1);
		f.write ((char*)&Word,2);
		Word-=0x80A0;
		if (Word) CutTail=f.tellp();
	 }
	SetEndOfFile((void*)_get_osfhandle(f.fd()));
	f.close();

	free(file);
}

int RGBPixel (char *Bitmap24bit, int x, int y, int W, int W32, int H, int Rb, int Gb, int Bb, int C, int Pos)
{
	x+=Pos%2;	//Actual Braille dot position by it's index
	y+=Pos/2;
	if (x<W && y<H) return Bitmap24bit [W32*(H-y-1) + x*3 + C];

	if (C==2) return Rb;
	if (C==1) return Gb;
	return Bb;
}

#define R(n) RGBPixel(file, x, y, X, LS, Y, Config.Bgnd[0], Config.Bgnd[1], Config.Bgnd[2], 2, n)
#define G(n) RGBPixel(file, x, y, X, LS, Y, Config.Bgnd[0], Config.Bgnd[1], Config.Bgnd[2], 1, n)
#define B(n) RGBPixel(file, x, y, X, LS, Y, Config.Bgnd[0], Config.Bgnd[1], Config.Bgnd[2], 0, n)

void BBCode (fstream *InFile, int Offset, int X, int Y, char *OutFile)
{
	fstream f;
	char *file;
	unsigned short Word;
	int x, y, LS;
	int mask, pos, mr,mg,mb, n,err,merr, BestFit[4], LastBest[4]={100500,100500,100500,0};
	int tailoptimise=0;

	LS=(X*3+3)&0xFFFFFFFC;
	file=(char*)malloc(Y*LS);
	InFile->seekg(Offset);
	InFile->read (file,Y*LS);
	InFile->close();

	f.open (OutFile,ios::out|ios::binary);
	for (y=0;y<Y;y+=4+Config.SY)
	{
		for (x=0;x<X;x+=2+Config.SX)
		{
			Word=0x80A0;
	
			merr=0x7FFFFFFF;
			for (mask=0;mask<256;mask++)
			{
				n=mr=mg=mb=0;
				for (pos=0;pos<8;pos++)
				 if (mask&(1<<pos)) n++, mr+=R(pos), mg+=G(pos), mb+=B(pos);
				if (n) mr/=n, mg/=n, mb/=n;
				err=0;
				for (pos=0;pos<8;pos++)
				 if (mask&(1<<pos)) err+=SqPifa( R(pos)-mr            , G(pos)-            mg, B(pos)-mb            );
				 else               err+=SqPifa( R(pos)-Config.Bgnd[0], G(pos)-Config.Bgnd[1], B(pos)-Config.Bgnd[2]);
				if (err<merr) merr=err,BestFit[3]=mask,BestFit[0]=mr,BestFit[1]=mg,BestFit[2]=mb;
			}
			if (BestFit[3]& 1 ) Word|=0x100;
			if (BestFit[3]& 4 ) Word|=0x200;
			if (BestFit[3]&16 ) Word|=0x400;
			if (BestFit[3]& 2 ) Word|=0x800;
			if (BestFit[3]& 8 ) Word|=0x1000;
			if (BestFit[3]&32 ) Word|=0x2000;
			if (BestFit[3]&64 ) Word|=0x1;
			if (BestFit[3]&128) Word|=0x2;
	
			if (BestFit[3])	//First of all, we have an actual character to type. Space can have any font color :)
			 if (SqPifa(BestFit[0]-LastBest[0],BestFit[1]-LastBest[1],BestFit[2]-LastBest[2])>Config.MaxDiff*Config.MaxDiff)	//Must change current color!
			 {
				if (LastBest[3]) f<<Config.BBCode[1];	//close current color tag
				if (SqPifa(BestFit[0]-Config.Fgnd[0],BestFit[1]-Config.Fgnd[1],BestFit[2]-Config.Fgnd[2])>Config.MaxDiff*Config.MaxDiff)	//Must set color other than default foregnd
				{
					for (pos=1,mask=0; mask<6&&pos<strlen(Config.BBCode[0]); pos++)
					 if (mask || Config.BBCode[0][pos-1]=='#')
					 {
						if (mask&1) Config.BBCode[0][pos]='0'+BestFit[mask/2]%16;
						else        Config.BBCode[0][pos]='0'+BestFit[mask/2]/16;
						mask++;
						if (Config.BBCode[0][pos]>'9') Config.BBCode[0][pos]+='A'-'9'-1;
					 }
					f<<Config.BBCode[0];
					LastBest[3]=1;	//remember: tag is open!
				} else LastBest[3]=0;	//remember: tag is closed, default color used!
				memcpy(LastBest, BestFit, sizeof(int)*3);	//remember new current color for RLE compression
			 }
	
			f.write (&ConstBraille,1);
			f.write ((char*)&Word,2);
			if (BestFit[3]) tailoptimise=f.tellp();	//there is something except empty spaces at the current line.
		}
		f.seekp(tailoptimise);
		f<<Config.BBCode[2];
		tailoptimise=f.tellp();
	}
	if (LastBest[3]) f<<Config.BBCode[1];	//close last color tag (if any)
	SetEndOfFile((void*)_get_osfhandle(f.fd()));
	f.close();

	free(file);
}

char* Go (char* InFile, char* OutFile, char *CfgFile)
{
	fstream f;
	BMP_HEADER Head;

	if (CfgFile)
	{
		f.open (CfgFile, ios::in|ios::binary);
		if (f.fail()) return "Can't open config file!";
		f.read ((char*)&Config,sizeof(Config));
		if (f.fail()) return "Can't read config file!";
		f.close();
	}

	f.open (InFile, ios::in|ios::binary);
	if (f.fail()) return "Can't open input file!";
	f.read ((char*)&Head, sizeof (Head));
	if (f.fail()) return "Can't read input file!";
	if (Head.Id!=0x4D42) return "Wrong BMP id! Not a BMP file?";
	if (Head.Compr) return "Compressed files are not supported!";
	if (Head.BPP==24)
	{
		BBCode (&f, Head.DataOffset, Head.Width, Head.Height, OutFile);
	}
	else if (Head.BPP==1)
	{
		Mono (&f, Head.DataOffset, Head.Width, Head.Height, Head.Palette[0][0], OutFile);
	}
	else return "Wrong bits per pixel! Only 1-bit monochrome (simple ASCII) and 24-bit RGB (ASCII + BBCode) files are supported!";

	return NULL;
}

char InFile[_MAX_PATH], OutFile[_MAX_PATH];
BOOL __export PASCAL WinFn (HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			SetDlgItemText(hWnd, 1012, "\n");
		return FALSE;

		case WM_CLOSE:
			EndDialog (hWnd,NULL);
		return FALSE;

		case WM_COMMAND:
			OPENFILENAME FName={sizeof(OPENFILENAME),hWnd};
			if (lParam && wParam==(BN_CLICKED<<16|1018))
			{
				strcpy (InFile,"BrArt.cfg");
				FName.lpstrFilter = "BrArt config file\0*.cfg\0\0";
				FName.lpstrFile = InFile;
				FName.nMaxFile = _MAX_PATH;
				FName.Flags = OFN_FILEMUSTEXIST|OFN_EXPLORER;
				FName.lpstrDefExt="CFG";
				FName.lpTemplateName="";
				if (!GetOpenFileName(&FName)) return TRUE;
				fstream f;
				f.open (InFile, ios::in|ios::binary);
				if (f.fail()) return TRUE;
				f.read ((char*)&Config,sizeof(Config));
				if (f.fail()) return TRUE;
				f.close();

				SetDlgItemInt(hWnd, 2002, Config.SX, FALSE);
				SetDlgItemInt(hWnd, 2003, Config.SY, FALSE);
				SetDlgItemText(hWnd, 1005, Config.BBCode[0]);
				SetDlgItemText(hWnd, 1010, Config.BBCode[1]);
				SetDlgItemText(hWnd, 1012, Config.BBCode[2]);
				SetDlgItemInt(hWnd, 1014, Config.MaxDiff, FALSE);
				sprintf (OutFile, "#%X%X%X%X%X%X", Config.Bgnd[0]/16, Config.Bgnd[0]%16, Config.Bgnd[1]/16, Config.Bgnd[1]%16, Config.Bgnd[2]/16, Config.Bgnd[2]%16);
				SetDlgItemText(hWnd, 2001, OutFile);
				sprintf (OutFile, "#%X%X%X%X%X%X", Config.Fgnd[0]/16, Config.Fgnd[0]%16, Config.Fgnd[1]/16, Config.Fgnd[1]%16, Config.Fgnd[2]/16, Config.Fgnd[2]%16);
				SetDlgItemText(hWnd, 1015, OutFile);

				cout<<"Config read.\n";
			}
			if (lParam && wParam==(BN_CLICKED<<16|1017))
			{
				strcpy (OutFile,"BrArt.cfg");
				FName.lpstrFilter = "BrArt config file\0*.cfg\0\0";
				FName.lpstrFile = OutFile;
				FName.nMaxFile = _MAX_PATH;
				FName.Flags = OFN_OVERWRITEPROMPT|OFN_EXPLORER;
				FName.lpstrDefExt="CFG";
				FName.lpTemplateName="";
				if (!GetOpenFileName(&FName)) return TRUE;

				Config.SX=GetDlgItemInt(hWnd, 2002, (int*)InFile, FALSE);
				Config.SY=GetDlgItemInt(hWnd, 2003, (int*)InFile, FALSE);
				GetDlgItemText(hWnd, 1005, Config.BBCode[0], 32);
				GetDlgItemText(hWnd, 1010, Config.BBCode[1], 32);
				GetDlgItemText(hWnd, 1012, Config.BBCode[2], 32);
				Config.MaxDiff=GetDlgItemInt(hWnd, 1014, (int*)InFile, FALSE);
				GetDlgItemText(hWnd, 2001, InFile, 12);
				sscanf (InFile, "#%x", Config.Bgnd);
				Config.Bgnd[2]=Config.Bgnd[0]&0xFF;
				Config.Bgnd[1]=Config.Bgnd[0]>>8&0xFF;
				Config.Bgnd[0]=Config.Bgnd[0]>>16&0xFF;
				GetDlgItemText(hWnd, 1015, InFile, 12);
				sscanf (InFile, "#%x", Config.Fgnd);
				Config.Fgnd[2]=Config.Fgnd[0]&0xFF;
				Config.Fgnd[1]=Config.Fgnd[0]>>8&0xFF;
				Config.Fgnd[0]=Config.Fgnd[0]>>16&0xFF;

				fstream f;
				f.open (OutFile, ios::out|ios::binary);
				if (f.fail()) return TRUE;
				f.write ((char*)&Config,sizeof(Config));
				if (f.fail()) return TRUE;
				f.close();
				cout<<"Config written.\n";
			}
			if (lParam && wParam==(BN_CLICKED<<16|2000))
			{
				Config.SX=GetDlgItemInt(hWnd, 2002, (int*)InFile, FALSE);
				Config.SY=GetDlgItemInt(hWnd, 2003, (int*)InFile, FALSE);
				GetDlgItemText(hWnd, 1005, Config.BBCode[0], 32);
				GetDlgItemText(hWnd, 1010, Config.BBCode[1], 32);
				GetDlgItemText(hWnd, 1012, Config.BBCode[2], 32);
				Config.MaxDiff=GetDlgItemInt(hWnd, 1014, (int*)InFile, FALSE);
				GetDlgItemText(hWnd, 2001, InFile, 12);
				sscanf (InFile, "#%x", Config.Bgnd);
				Config.Bgnd[2]=Config.Bgnd[0]&0xFF;
				Config.Bgnd[1]=Config.Bgnd[0]>>8&0xFF;
				Config.Bgnd[0]=Config.Bgnd[0]>>16&0xFF;
				GetDlgItemText(hWnd, 1015, InFile, 12);
				sscanf (InFile, "#%x", Config.Fgnd);
				Config.Fgnd[2]=Config.Fgnd[0]&0xFF;
				Config.Fgnd[1]=Config.Fgnd[0]>>8&0xFF;
				Config.Fgnd[0]=Config.Fgnd[0]>>16&0xFF;

				strcpy (InFile,"*.bmp");
				FName.lpstrFilter = "Windows Bitmap file (1-bit B&W or 24-bit RGB)\0*.bmp\0\0";
				FName.lpstrFile = InFile;
				FName.nMaxFile = _MAX_PATH;
				FName.Flags = OFN_FILEMUSTEXIST|OFN_EXPLORER;
				FName.lpstrDefExt="BMP";
				FName.lpTemplateName="";
				if (!GetOpenFileName(&FName)) return TRUE;

				strcpy (OutFile,InFile);
				strcat (OutFile, ".TXT");
				FName.lpstrFilter = "UTF-8 text file\0*.txt\0UTF-8 HTML file\0*.htm\0\0";
				FName.lpstrFile = OutFile;
				FName.nMaxFile = _MAX_PATH;
				FName.Flags = OFN_OVERWRITEPROMPT|OFN_EXPLORER;
				FName.lpstrDefExt=NULL;
				FName.lpTemplateName="";
				if (!GetOpenFileName(&FName)) return TRUE;

				char *Error;
				if (Error = Go(InFile, OutFile, NULL)) cout<<Error<<endl;
				else cout<<"Conversion completed"<<endl;
			}
		break;

		default:
		return FALSE;
	}
	return TRUE;
}

void main (int argc, char* argv[])
{
	char *Error;

	cout<<"Automated/Batch processing (command line parameters):\nBrArt image.bmp destination.txt parameters.cfg\nMonochrome files become plain text, RGB files become BBCode.\n";
	if (argc==4)
	{
		if (Error = Go(argv[1], argv[2], argv[3])) cout<<Error<<endl;
	}
	else	DialogBoxParam(GetModuleHandle(NULL),"MAINWIN",NULL,WinFn,NULL);
}