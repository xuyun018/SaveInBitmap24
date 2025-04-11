#include <Windows.h>

#include <stdint.h>
#include <math.h>

#include <stdio.h>

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

inline
unsigned char *write_uint32(unsigned char *buffer, uint32_t value)
{
	(*buffer++) = (value >> 24);
	(*buffer++) = (value >> 16);
	(*buffer++) = (value >> 8);
	(*buffer++) = value;
	return(buffer);
}
inline
uint32_t read_uint32(const unsigned char *buffer)
{
	uint32_t result = 0;

	result = (result << 8) | (*buffer++);
	result = (result << 8) | (*buffer++);
	result = (result << 8) | (*buffer++);
	result = (result << 8) | (*buffer++);
	return(result);
}

unsigned char *ReadFileBuffer(const TCHAR *filename, unsigned int *filesize)
{
	HANDLE hfile;
	DWORD numberofbytes;
	unsigned char *result = NULL;

	hfile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile != INVALID_HANDLE_VALUE)
	{
		*filesize = GetFileSize(hfile, NULL);

		result = (unsigned char *)MALLOC(*filesize);
		if (result != NULL)
		{
			ReadFile(hfile, result, *filesize, &numberofbytes, NULL);
		}

		CloseHandle(hfile);
	}
	return(result);
}
BOOL WriteFileBuffer(const TCHAR *filename, unsigned char *filebuffer, unsigned int filesize)
{
	HANDLE hfile;
	DWORD numberofbytes;
	BOOL result = FALSE;

	hfile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile != INVALID_HANDLE_VALUE)
	{
		WriteFile(hfile, filebuffer, filesize, &numberofbytes, NULL);

		CloseHandle(hfile);
	}
	return(result);
}
BOOL WriteFileBuffer1(const TCHAR *filename, unsigned char *filebuffer, unsigned int filesize)
{
	HANDLE hfile;
	DWORD numberofbytes;
	BOOL result = FALSE;

	hfile = CreateFile(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER li;

		li.QuadPart = 0;
		SetFilePointerEx(hfile, li, NULL, FILE_END);

		WriteFile(hfile, filebuffer, filesize, &numberofbytes, NULL);

		CloseHandle(hfile);
	}
	return(result);
}

void SaveToBitmap24File(const WCHAR *filename, BITMAPINFOHEADER *pbmih, LPVOID bits)
{
	BITMAPFILEHEADER pbmfh[1];

	pbmfh->bfType = 19778; // always the same, 'BM'
	pbmfh->bfReserved1 = pbmfh->bfReserved2 = 0;
	pbmfh->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	pbmfh->bfSize = pbmih->biSizeImage;

	HANDLE hfile;
	DWORD numberofbytes;
	BOOL result = FALSE;

	hfile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hfile != INVALID_HANDLE_VALUE)
	{
		WriteFile(hfile, (unsigned char *)pbmfh, sizeof(BITMAPFILEHEADER), &numberofbytes, NULL);
		WriteFile(hfile, (unsigned char *)pbmih, pbmih->biSize, &numberofbytes, NULL);
		WriteFile(hfile, (unsigned char *)bits, pbmih->biSizeImage, &numberofbytes, NULL);

		CloseHandle(hfile);
	}
}

int write_to_bitmap24(int argc, WCHAR *argv[])
{
	BITMAPFILEHEADER *pbmfh;
	BITMAPINFOHEADER *pbmih;
	unsigned char *bits;
	unsigned char *buf;
	unsigned char *dst;
	unsigned char *_dst;
	unsigned int len;
	unsigned int dst_len;
	unsigned int stride;
	unsigned int h;
	unsigned int i;
	unsigned int j;
	unsigned int k;
	unsigned int l;
	unsigned int width, height;

	if (argc > 3)
	{
		buf = ReadFileBuffer(argv[2], &len);
		if (buf)
		{
			dst_len = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 4 + len;
			dst = (unsigned char *)MALLOC(dst_len);
			if (dst)
			{
				_dst = dst;
				pbmfh = (BITMAPFILEHEADER *)_dst;
				_dst += sizeof(BITMAPFILEHEADER);

				pbmfh->bfType = 19778; // always the same, 'BM'
				pbmfh->bfReserved1 = pbmfh->bfReserved2 = 0;
				pbmfh->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
				//pbmfh->bfSize;

				pbmih = (BITMAPINFOHEADER *)_dst;
				_dst += sizeof(BITMAPINFOHEADER);
				bits = _dst;

				i = sqrt((double)((len + 2) / 3));
				i++;
				j = ((len + 2) / 3) / i;
				j++;
				j = (j + 3) & ~3;

				width = j;
				height = i;

				pbmih->biSize = sizeof(BITMAPINFOHEADER);
				pbmih->biWidth = width;
				pbmih->biHeight = height;
				pbmih->biPlanes = 1;
				pbmih->biBitCount = 0x18;
				pbmih->biCompression = 0;
				//pbmih->biSizeImage;
				pbmih->biXPelsPerMeter = 0;
				pbmih->biYPelsPerMeter = 0;
				pbmih->biClrUsed = 0;
				pbmih->biClrImportant = 0;

				stride = ((width * 24 + 31) & ~31) >> 3;

				pbmih->biSizeImage = stride * height;

				pbmfh->bfSize = pbmih->biSizeImage;

				write_uint32(bits, len);
				memcpy(bits + 4, buf, len);

				WriteFileBuffer(argv[3], dst, dst_len);
				WriteFileBuffer1(argv[3], dst, pbmih->biSizeImage - 4 - len);

				FREE(dst);
			}

			FREE(buf);
		}
	}

	return(0);
}
int read_from_bitmap24(int argc, WCHAR *argv[])
{
	BITMAPINFOHEADER *pbmih;
	unsigned char *bits;
	unsigned char *buf;
	unsigned char *_buf;
	unsigned int len;
	unsigned int l;

	if (argc > 3)
	{
		buf = ReadFileBuffer(argv[2], &len);
		if (buf)
		{
			_buf = buf + sizeof(BITMAPFILEHEADER);

			pbmih = (BITMAPINFOHEADER *)_buf;
			_buf += sizeof(BITMAPINFOHEADER);
			bits = _buf;

			l = read_uint32(bits);
			if (4 + l <= pbmih->biSizeImage)
			{
				WriteFileBuffer(argv[3], bits + 4, l);
			}

			FREE(buf);
		}
	}

	return(0);
}

int wmain(int argc, WCHAR *argv[])
{
	WCHAR *str;

	if (argc > 3)
	{
		str = argv[1];

		if (str[0] = '-')
		{
			if (str[1] == 'E' || str[1] == 'e')
			{
				write_to_bitmap24(argc, argv);
			}
			else if (str[1] == 'D' || str[1] == 'd')
			{
				read_from_bitmap24(argc, argv);
			}
		}
	}

	return(0);
}