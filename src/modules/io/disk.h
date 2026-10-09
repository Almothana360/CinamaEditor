#ifndef CE_MODULES_IO_DISK_H
#define CE_MODULES_IO_DISK_H

#include "modules/buffer/document.h"
#include <stdbool.h>

// Binds the disk subsystem to a document to listen for file actions on the Event Bus.
void Disk_Init(Document *doc);

// Performs a blocking read from the file system.
bool Disk_LoadFile(Document *doc, const char *filepath);

// Flushes the current document state back to the disk.
bool Disk_SaveFile(Document *doc);

#endif // CE_MODULES_IO_DISK_H