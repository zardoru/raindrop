
#include "OffsetDatabase.h"

auto qOffsetDbDDL =
"CREATE TABLE[offsets](\
	[charthash] varchar(64) NOT NULL,\
	[diffindex] int NOT NULL DEFAULT 0,\
	[offset] double NOT NULL UNIQUE DEFAULT 0,\
	[judgeoffset] double NOT NULL DEFAULT 0);\
\
CREATE INDEX[chartfile]\
ON[offsets](\
	[charthash],\
	[diffindex]);";