#define _CRT_SECURE_NO_WARNINGS
#include <direct.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

char *DirName;
char DirNameBuf[260];
size_t DirNameLen;
char *Name;
size_t NameLen;

int PakCreate() {
	FILE *addfile = NULL;
	unsigned char bytes[0x40000];
	char filename[260];
	char *filenameptr;
	long filelen;
	int i;
	FILE *listfile;
	int namebyte;
	unsigned int numfiles = 0;
	long oldfilelen;
	FILE *pakfile;

	memcpy(filename, DirName, DirNameLen);
	filename[DirNameLen] = '.';
	filename[DirNameLen + 4] = 0;

	filename[DirNameLen + 1] = 't';
	filename[DirNameLen + 2] = 'x';
	filename[DirNameLen + 3] = 't';
	listfile = fopen(filename, "r");
	if (!listfile) {
		fputs("Failed to open list file\n", stderr);
		return 1;
	}
	filename[DirNameLen + 1] = 'p';
	filename[DirNameLen + 2] = 'a';
	filename[DirNameLen + 3] = 'k';
	pakfile = fopen(filename, "wb");
	if (!pakfile) {
		fputs("Failed to open .pak file\n", stderr);
		return 1;
	}

	if (!fwrite(&numfiles, 4, 1, pakfile)) goto writefail;

	for (;;) {
		filenameptr = filename;
		for (i = 0; i < 255; ++i) {
			namebyte = fgetc(listfile);
			if ((namebyte < 0) || (namebyte == '\n'))
				break;
			*(filenameptr++) = namebyte;
		}
		if (filenameptr == filename) {
			if (namebyte < 0)
				break;
			continue;
		}
		*((unsigned int *)(filenameptr)) = 'zef.';
		filenameptr[4] = 0;
		addfile = fopen(filename, "rb");
		*filenameptr = 0;
		if (!addfile) goto filefail;
		++numfiles;
		filenameptr = filename;
		while (*filenameptr) {
			if (*filenameptr == '/')
				*filenameptr = '\\';
			++filenameptr;
		}
		if (fputc(filenameptr - filename, pakfile) < 0) goto writefail;
		if (!fwrite(filename, filenameptr - filename, 1, pakfile)) goto writefail;
		fseek(addfile, 0, SEEK_END);
		filelen = ftell(addfile);
		if (!fwrite(&filelen, 4, 1, pakfile)) goto writefail;
		rewind(addfile);
		while (filelen & 0xfffc0000) {
			if (!fread(bytes, 0x40000, 1, addfile)) goto readfail;
			filelen -= 0x40000;
			if (!fwrite(bytes, 0x40000, 1, pakfile)) goto writefail;
		}
		if (filelen) {
			if (!fread(bytes, filelen, 1, addfile)) goto readfail;
			oldfilelen = filelen;
			filelen = 0;
			if (!fwrite(bytes, oldfilelen, 1, pakfile)) goto writefail;
		}
		fclose(addfile);
		addfile = NULL;
		printf("OK %s.fez\n", filename);
		continue;
filefail:
		printf("FAIL %s.fez\n", filename);
		if (addfile) {
			fclose(addfile);
			addfile = NULL;
		}
	}
	rewind(pakfile);
	if (!fwrite(&numfiles, 4, 1, pakfile)) goto writefail;
	fflush(pakfile);
	fclose(pakfile);
	fclose(listfile);
	return 0;
readfail:
	fprintf(stderr, "Failed to read from %s\n", filename);
	goto fail;
writefail:
	fputs("Failed to write to .pak file\n", stderr);
fail:
	if (addfile)
		fclose(addfile);
	fclose(pakfile);
	fclose(listfile);
	return 1;
}

int PakCreatePath(char *path, size_t pathlen) {
	char newpathbuf[520];
	char *newpath = newpathbuf;

	while (pathlen--) {
		if (*path == '\\') {
			*newpath = 0;
			if (_mkdir(newpathbuf)) {
				if (errno != EEXIST)
					return 1;
			}
		}
		*(newpath++) = *(path++);
	}
	return 0;
}

int PakExtract() {
	unsigned char bytes[0x40000];
	int dirnamelen = DirNameLen + 1;
	unsigned int filelen;
	char filename[520];
	int filenamelen;
	FILE *listfile = NULL;
	unsigned int numfiles;
	unsigned int oldfilelen;
	FILE *pakfile;
	FILE *targetfile = NULL;

	pakfile = fopen(Name, "rb");
	if (!pakfile) {
		fputs("Failed to open .pak file\n", stderr);
		return 1;
	}
	if (!fread(&numfiles, 4, 1, pakfile))
		goto readfail;
	
	memcpy(filename, DirName, DirNameLen);
	*((unsigned int *)(filename + DirNameLen)) = 'txt.';
	filename[DirNameLen + 4] = 0;
	listfile = fopen(filename, "w");

	filename[DirNameLen] = '\\';

	while (numfiles--) {
		filenamelen = fgetc(pakfile);
		if (filenamelen < 0) goto readfail;
		if (!fread(filename + dirnamelen, filenamelen, 1, pakfile)) goto readfail;
		filenamelen += dirnamelen;
		*((unsigned int *)(filename + filenamelen)) = 'zef.';
		filenamelen += 4;
		filename[filenamelen] = 0;
		if (!fread(&filelen, 4, 1, pakfile)) goto readfail;

		if (PakCreatePath(filename, filenamelen)) goto filefail;
		targetfile = fopen(filename, "wb");
		if (!targetfile) goto filefail;

		while (filelen & 0xfffc0000) {
			if (!fread(bytes, 0x40000, 1, pakfile)) goto readfail;
			filelen -= 0x40000;
			if (!fwrite(bytes, 0x40000, 1, targetfile)) goto filefail;
		}
		if (filelen) {
			if (!fread(bytes, filelen, 1, pakfile)) goto readfail;
			oldfilelen = filelen;
			filelen = 0;
			if (!fwrite(bytes, oldfilelen, 1, targetfile)) goto filefail;
		}

		fflush(targetfile);
		fclose(targetfile);
		targetfile = NULL;
		filename[filenamelen - 4] = 0;
		if (listfile)
			fprintf(listfile, "%s\n", filename + dirnamelen);
		printf("OK %s.fez\n", filename);
		continue;
filefail:
		printf("FAIL %s\n", filename);
		fseek(pakfile, filelen, SEEK_CUR);
		if (targetfile) {
			fclose(targetfile);
			targetfile = NULL;
		}
	}

	if (listfile)
		fclose(listfile);
	fclose(pakfile);
	return 0;
readfail:
	if (targetfile) {
		fflush(targetfile);
		fclose(targetfile);
	}
	if (listfile)
		fclose(listfile);
	fclose(pakfile);
	fputs("Failed to read from .pak file\n", stderr);
	return 1;
}

int main(int argc, char **argv) {
	if (argc <= 1)
		goto invalidarg;
	Name = argv[1];
	NameLen = strlen(Name);
	if (NameLen < 5)
		goto invalidarg;
	if ((Name[NameLen - 5] == '\\') || (Name[NameLen - 5] == '/'))
		goto invalidarg;
	if (Name[NameLen - 4] != '.')
		goto invalidarg;
	{
		unsigned int dirnamestart = NameLen - 4;
		char *dirnameptr = Name + dirnamestart;

		DirNameBuf[259] = 0;
		DirName = DirNameBuf + 259;
		while (dirnamestart--) {
			--dirnameptr;
			if ((*dirnameptr == '\\') || (*dirnameptr == '/'))
				break;
			*(--DirName) = *dirnameptr;
		}
		DirNameLen = strlen(DirName);
	}
	if (((Name[NameLen - 3] == 't') || (Name[NameLen - 3] == 'T')) &&
		((Name[NameLen - 2] == 'x') || (Name[NameLen - 2] == 'X')) &&
		((Name[NameLen - 1] == 't') || (Name[NameLen - 1] == 'T')))
		return PakCreate();
	if (((Name[NameLen - 3] == 'p') || (Name[NameLen - 3] == 'P')) &&
		((Name[NameLen - 2] == 'a') || (Name[NameLen - 2] == 'A')) &&
		((Name[NameLen - 1] == 'k') || (Name[NameLen - 1] == 'K')))
		return PakExtract();
invalidarg:
	fputs("FEZ .pak creator/extractor\nDrag-and-drop .pak to extract or .txt with a list of files to pack\n", stderr);
	return 1;
}