#include "xport.h"
#include "eaclib/psx/eaclib.h"
#include "game/common/minfront.h"

sint32 * MinFront::ParseOptions(void)
{
    FUNCTION_MARKER(0x800128F0, "FRONT.BIN");
    sint32 *stream = (sint32 *)reservememadr("FE Data Stream", 0x2000, 0x10U);
    *stream = 0;
    resizememadr(stream, 4);
    return stream;
}
