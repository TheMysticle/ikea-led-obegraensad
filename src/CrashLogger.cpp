#include "CrashLogger.h"
#include <Preferences.h>

#ifdef ESP32
#include <esp_system.h>
#endif
#include "config.h"

Preferences crashPrefs;

#ifdef ESP32
#define RTC_LOG_SIZE 2048
RTC_NOINIT_ATTR char rtc_panic_log[RTC_LOG_SIZE];
RTC_NOINIT_ATTR uint32_t rtc_panic_log_idx;
RTC_NOINIT_ATTR uint32_t rtc_panic_log_magic;

bool rtc_panic_log_enabled = false;

extern "C" void __real_panic_print_char(const char c);
extern "C" void __wrap_panic_print_char(const char c) {
    if (rtc_panic_log_enabled && rtc_panic_log_magic == 0x12345678 && rtc_panic_log_idx < RTC_LOG_SIZE - 1) {
        rtc_panic_log[rtc_panic_log_idx++] = c;
    }
    __real_panic_print_char(c);
}

extern "C" void __real_panic_print_str(const char *str);
extern "C" void __wrap_panic_print_str(const char *str) {
    if (rtc_panic_log_enabled && rtc_panic_log_magic == 0x12345678) {
        while (*str && rtc_panic_log_idx < RTC_LOG_SIZE - 1) {
            rtc_panic_log[rtc_panic_log_idx++] = *str++;
        }
    }
    __real_panic_print_str(str);
}

extern "C" void __real_panic_print_hex(int h);
extern "C" void __wrap_panic_print_hex(int h) {
    if (rtc_panic_log_enabled && rtc_panic_log_magic == 0x12345678) {
        for (int i = 28; i >= 0; i -= 4) {
            int val = (h >> i) & 0xF;
            char c = val < 10 ? '0' + val : 'a' + val - 10;
            if (rtc_panic_log_idx < RTC_LOG_SIZE - 1) {
                rtc_panic_log[rtc_panic_log_idx++] = c;
            }
        }
    }
    __real_panic_print_hex(h);
}

extern "C" void __real_panic_print_dec(int d);
extern "C" void __wrap_panic_print_dec(int d) {
    if (rtc_panic_log_enabled && rtc_panic_log_magic == 0x12345678) {
        if (d == 0) {
            if (rtc_panic_log_idx < RTC_LOG_SIZE - 1) rtc_panic_log[rtc_panic_log_idx++] = '0';
        } else {
            char buf[16];
            int pos = 0;
            unsigned int ud = d;
            if (d < 0) {
                if (rtc_panic_log_idx < RTC_LOG_SIZE - 1) rtc_panic_log[rtc_panic_log_idx++] = '-';
                ud = -d;
            }
            while (ud > 0) {
                buf[pos++] = '0' + (ud % 10);
                ud /= 10;
            }
            while (pos > 0 && rtc_panic_log_idx < RTC_LOG_SIZE - 1) {
                rtc_panic_log[rtc_panic_log_idx++] = buf[--pos];
            }
        }
    }
    __real_panic_print_dec(d);
}
#endif

void CrashLogger::init() {
#ifdef ESP32
    // Read config to see if we should be catching panics
    rtc_panic_log_enabled = config.getCrashReportingEnabled();
    
    if (!rtc_panic_log_enabled) {
        return; // Completely disabled, do not read or reset the buffer
    }

    esp_reset_reason_t reason = esp_reset_reason();
    String reasonStr = "Unknown";
    bool isCrash = false;

    switch (reason) {
        case ESP_RST_POWERON:   reasonStr = "Power-on"; break;
        case ESP_RST_EXT:       reasonStr = "External Pin"; break;
        case ESP_RST_SW:        reasonStr = "Software Reset"; break;
        case ESP_RST_PANIC:     reasonStr = "Exception/Panic"; isCrash = true; break;
        case ESP_RST_INT_WDT:   reasonStr = "Interrupt Watchdog"; isCrash = true; break;
        case ESP_RST_TASK_WDT:  reasonStr = "Task Watchdog"; isCrash = true; break;
        case ESP_RST_WDT:       reasonStr = "Other Watchdog"; isCrash = true; break;
        case ESP_RST_DEEPSLEEP: reasonStr = "Deep Sleep"; break;
        case ESP_RST_BROWNOUT:  reasonStr = "Brownout (Low Voltage)"; isCrash = true; break;
        case ESP_RST_SDIO:      reasonStr = "SDIO"; break;
        default:                reasonStr = "Other (" + String((int)reason) + ")"; break;
    }

    if (isCrash) {
        crashPrefs.begin("diagnostics", false);
        crashPrefs.putString("lastCrash", reasonStr);
        if (rtc_panic_log_magic == 0x12345678 && rtc_panic_log_idx > 0 && rtc_panic_log_idx < RTC_LOG_SIZE) {
            rtc_panic_log[rtc_panic_log_idx] = '\0';
            crashPrefs.putString("lastBacktrace", String(rtc_panic_log));
        }
        crashPrefs.end();
        Serial.println("[CrashLogger] Detected previous crash: " + reasonStr);
    }
    
    // Reset RTC magic and index to capture next crash
    rtc_panic_log_magic = 0x12345678;
    rtc_panic_log_idx = 0;
    memset(rtc_panic_log, 0, RTC_LOG_SIZE);
#endif
}

String CrashLogger::getLastCrashReason() {
    crashPrefs.begin("diagnostics", true);
    String reason = crashPrefs.getString("lastCrash", "No recent crashes");
    crashPrefs.end();
    return reason;
}

String CrashLogger::getLastBacktrace() {
    crashPrefs.begin("diagnostics", true);
    String backtrace = crashPrefs.getString("lastBacktrace", "");
    crashPrefs.end();
    return backtrace;
}

void CrashLogger::clearLastCrashReason() {
    crashPrefs.begin("diagnostics", false);
    crashPrefs.remove("lastCrash");
    crashPrefs.remove("lastBacktrace");
    crashPrefs.end();
}
