#pragma once

// The ZX Spectrum Next's two UARTs (0 ESP, 1 Pi) carried to serial ports of the host:
// next.esp/next.pi in [MEDIA], or --uart-esp/--uart-pi for one run. UART 0 may also be
// the emulated ESP-01 (SERIAL_ESP), which it is unless the user says otherwise.

#include <QStringList>

struct Computer;

#define SERIAL_ESP	"esp"		// the name that stands for the emulated ESP-01

void serial_frame(Computer* comp);		// the emulation thread, at each frame end
// the port a UART goes to, empty: none - the config's; user: picked now, which also
// replaces the command line's
void serial_set(int uart, const QString& name, bool user = false);
void serial_cli(int uart, const QString& name);	// for this run only, over the config
QString serial_name(int uart);			// what it goes to: the command line's, else the config's
QString serial_conf(int uart);			// the config's
QStringList serial_ports();			// the host's serial ports, for a list to pick from
void serial_close();
