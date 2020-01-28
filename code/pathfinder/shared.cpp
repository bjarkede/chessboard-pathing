#include "shared.hpp"

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

bool ShouldPrintHelp(int argc, char** argv) {
  if (argc == 1)
    return true;

  return strcmp(argv[1], "/?") == 0 ||
         strcmp(argv[1], "/help") == 0 ||
         strcmp(argv[1], "--help") == 0;
};

const char* GetExecutableFileName(char* argv0) {
  static char fileName[256];

  char* s = argv0 + strlen(argv0);
  char* end = s;
  char* start = s;
  bool endFound = false;
  bool startFound = false;

  while (s >= argv0) {
    const char c = *s;
    if (!startFound && !endFound && c == '.')
      {
	end = s;
	endFound = true;
      }
    if (!startFound && (c == '\\' || c == '/'))
      {
	start = s + 1;
	break;
      }
    --s;
  }

  strncpy(fileName, start, (size_t)(end - start));
  fileName[sizeof(fileName) - 1] = '\0';

  return fileName;
};

bool WriteStepsToFile(struct File *file, int x, int y) {
  char output_buffer[32];
  sprintf(output_buffer, "%d,%d -> ", x, y);
  return file->Write(output_buffer, strlen(output_buffer));
}

void PrintDebug(const char* format, ...) {
  char msg[1024];

  va_list ap;
  va_start(ap, format);
  vsprintf(msg, format, ap);
  va_end(ap);

  fprintf(stderr, "DEBUG: %s", msg);
}

void PrintError(const char* format, ...) {
  char msg[1024];

  va_list ap;
  va_start(ap, format);
  vsprintf(msg, format, ap);
  va_end(ap);

  fprintf(stderr, "ERROR: %s", msg);
}

void FatalError(const char* format, ...) {
  char msg[1024];

  va_list ap;
  va_start(ap, format);
  vsprintf(msg, format, ap);
  va_end(ap);

  fprintf(stderr, "FATAL ERROR: %s", msg);

  exit(1);
}

File::File() : file_buffer(NULL) {};

File::~File() {
  if (file_buffer != NULL)
    fclose((FILE*)file_buffer);
}

bool File::Open(const char* filePath, const char* mode) {
  file_buffer = fopen(filePath, mode);
  return file_buffer != NULL;
}

bool File::Close() {
  return fclose((FILE*)file_buffer);
}

bool File::IsValid() {
  return file_buffer != NULL;
}

bool File::Read(void* data, size_t bytes) {
  return fread(data, bytes, 1, (FILE*)file_buffer) == 1;
}

bool File::Write(const void* data, size_t bytes) {
  bool result = fwrite(data, sizeof(char), bytes, (FILE*)file_buffer) == 1;
  fflush((FILE*)file_buffer);
  return result;
}

bool AllocBuffer(Buffer& buffer, uptr bytes)
{
	buffer.buffer = malloc(bytes);
	if(!buffer.buffer)
	{
		return false;
	}

	buffer.length = bytes;
	return true;
}

bool ReadEntireFile(Buffer& buffer, const char* filePath) {
	FILE* file = fopen(filePath, "rb");
	if(!file)
	{
		return false;
	}

	fseek(file, 0, SEEK_END);
	const uptr bufferLength = (uptr)ftell(file);
	if(!AllocBuffer(buffer, bufferLength + 1))
	{
		fclose(file);
		return false;
	}

	fseek(file, 0, SEEK_SET);
	if(fread(buffer.buffer, (size_t)bufferLength, 1, file) != 1)
	{
		fclose(file);
		return false;
	}

	fclose(file);
	((char*)buffer.buffer)[bufferLength] = '\0';
	return true;
}
