#include "dtc.h"

#include <cstdio>
#include <cstdlib>

#include "vesc_skin_view.h"
namespace core::dtc {
void format(uint16_t code, char* out, size_t len) { snprintf(out, len, "V%03u", code); }
bool parse(const char* text, uint16_t& code) {
  if (!text || text[0] != 'V') return false;
  char* end;
  auto value = strtoul(text + 1, &end, 10);
  if (*end || value > 255) return false;
  code = value;
  return true;
}
System system_of(uint16_t) { return System::Powertrain; }
Str system_name(uint16_t) { return Str::DTC_SYS_POWERTRAIN; }
bool is_generic(uint16_t) { return true; }
Severity severity_of(uint16_t code) { return code ? Severity::Warning : Severity::Normal; }
const char* describe(uint16_t code) { return VescSkinView::FaultText(code); }
}  // namespace core::dtc
