//
//                 mxToolKit (c) 1999 by Mete Ciragan
//
// file:           mxTga.cpp
// implementation: all
// last modified:  Apr 15 1999, Mete Ciragan
// copyright:      The programs and associated files contained in this
//                 distribution were developed by Mete Ciragan. The programs
//                 are not in the public domain, but they are freely
//                 distributable without licensing fees. These programs are
//                 provided without guarantee or warrantee expressed or
//                 implied.
//
#include <mx/mxTga.h>
#include <stdio.h>
#include <stdlib.h>



mxImage *
mxTgaRead (const char *filename)
{
#ifdef KINGPIN //tga RLE and 32bit loading
	byte bufIdx = 0, RLE = 0;
	byte buff[3] = { 0, 0,0 };
#endif
	FILE *file;
	file = fopen (filename, "rb");
	if (!file)
		return 0;

	byte identFieldLength;
	byte colorMapType;
	byte imageTypeCode;
	fread (&identFieldLength, sizeof (byte), 1, file);
	fread (&colorMapType, sizeof (byte), 1, file);
	fread (&imageTypeCode, sizeof (byte), 1, file);

	fseek (file, 12, SEEK_SET);

	word width, height;
	byte pixelSize;
	fread (&width, sizeof (word), 1, file);
	fread (&height, sizeof (word), 1, file);
	fread (&pixelSize, sizeof (byte), 1, file);

	// only 24-bit RGB uncompressed
	if (colorMapType != 0 ||
#ifdef KINGPIN
		(imageTypeCode != 2 && imageTypeCode != 10)||
		(pixelSize != 24 && pixelSize != 32)
#else
		imageTypeCode != 2 ||
		pixelSize != 24
#endif
		)
	{
		fclose (file);
		return 0;
	}

	fseek (file, 18 + identFieldLength, SEEK_SET);

	mxImage *image = new mxImage ();
	if (!image->create (width, height, 24))
	{
		delete image;
		fclose (file);
		return 0;
	}

	byte *data = (byte *) image->data;
	for (int y = 0; y < height; y++)
	{
		byte *scanline = (byte *) &data[(height - y - 1) * width * 3];
		for (int x = 0; x < width; x++)
		{
#ifdef KINGPIN
			if (imageTypeCode == 10)
			{
				if (bufIdx == 0 || !RLE)
				{
					if (bufIdx == 0) {
						bufIdx = fgetc(file); //get repetition count
						RLE = (bufIdx & 128) ? 1 : 0;
						bufIdx &= ~128;
					}
					else
						bufIdx--; //allow 1 extra pixel

					fread(&buff, sizeof(buff), 1, file); //get RGB value
					if (pixelSize == 32)
						fgetc(file); //discard alpha data
				}
				else //allow 1 extra pixel
					bufIdx--; //+127 Run-length Packet.

				scanline[x * 3 + 2] = buff[0];
				scanline[x * 3 + 1] = buff[1];
				scanline[x * 3 + 0] = buff[2];
			} 
			else {
#endif
			scanline[x * 3 + 2] = (byte)fgetc(file);
			scanline[x * 3 + 1] = (byte)fgetc(file);
			scanline[x * 3 + 0] = (byte)fgetc(file);
			//scanline[x * 4 + 3] = 0xff;
#ifdef KINGPIN
			if (pixelSize == 32)
				fgetc (file); //discard alpha data
			}
#endif
		}
	}

	fclose (file);

	return image;
}



bool
mxTgaWrite (const char *filename, mxImage *image)
{
	if (!image)
		return false;

	if (image->bpp != 24)
		return false;

	FILE *file = fopen (filename, "wb");
	if (!file)
		return false;

	//
	// write header
	//
	fputc (0, file); // identFieldLength
	fputc (0, file); // colorMapType == 0, no color map
	fputc (2, file); // imageTypeCode == 2, uncompressed RGB

	word w = 0;
	fwrite (&w, sizeof (word), 1, file); // colorMapOrigin
	fwrite (&w, sizeof (word), 1, file); // colorMapLength
	fputc (0, file); // colorMapEntrySize

	fwrite (&w, sizeof (word), 1, file); // imageOriginX
	fwrite (&w, sizeof (word), 1, file); // imageOriginY

	w = (word) image->width;
	fwrite (&w, sizeof (word), 1, file); // imageWidth

	w = (word) image->height;
	fwrite (&w, sizeof (word), 1, file); // imageHeight

	fputc (24, file); // imagePixelSize
	fputc (0, file); // imageDescriptorByte

	// write no ident field

	// write no color map

	// write imagedata
#ifndef KINGPIN
	byte *data = (byte *) image->data;
	for (int y = 0; y < image->height; y++)
	{
		byte *scanline = (byte *) &data[(image->height - y - 1) * image->width * 3];
		for (int x = 0; x < image->width; x++)
		{
			fputc ((byte) scanline[x * 3 + 2], file);
			fputc ((byte) scanline[x * 3 + 1], file);
			fputc ((byte) scanline[x * 3 + 0], file);
		}
	}
#else
	//hypov8 write without fliping Y axis
	fwrite(image->data, (image->height*image->width * 3), 1, file);
#endif
	fclose (file);

	return true;
}