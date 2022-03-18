#ifndef __JSONUI_H_
#define __JSONUI_H_

#include <string.h>

#include "nanoprintf.h"


// Extracted and modified from nanoprintf.h -- the implementation of npf_vsnprintf
// has an indirect function call which adds extra unnecessary size to the binary.
int snprintf_wrapper(char *buffer, size_t bufsz, const char *format, ...) {
  va_list val;
  va_start(val, format);
  npf_bufputc_ctx_t bufputc_ctx;
  bufputc_ctx.dst = buffer;
  bufputc_ctx.len = bufsz;
  bufputc_ctx.cur = 0;
  int const n = npf_vpprintf(npf_bufputc, &bufputc_ctx, format, val);
  npf_bufputc('\0', &bufputc_ctx);
  va_end(val);
  return n;
}


struct _remove_comma{};
_remove_comma remove_comma{};


class stringstream {
public:
    stringstream(char* buf) : _head(buf), _tail(buf) {}

    stringstream& operator<<(const char* s) {
        auto written = snprintf_wrapper(_tail, strlen(s) + 1, "%s", s);
        _tail += written;
        return *this;
    }

    stringstream& operator<<(int d) {
        auto written = snprintf_wrapper(_tail, 30, "%d", d);
        _tail += written;
        return *this;
    }

    stringstream& operator<<(float f) {
        auto written = snprintf_wrapper(_tail, 30, "%f", f);
        _tail += written;
        return *this;
    }

    stringstream& operator<<(const stringstream& other) {
        auto written = snprintf_wrapper(_tail, strlen(other._head) + 1, "%s", other._head);
        _tail += written;
        return *this;
    }

    // Remove a trailing comma if there is one.
    // e.g. ss << "[" << "1," << "2," << "3," << remove_comma << "]";
    // and  ss << "[" << remove_comma << "]";
    // both produce valid JSON, namely "[1,2,3]" and "[]" respectively.
    stringstream& operator<<(const _remove_comma&) {
        if (*(_tail-1) == ',') {
            _tail -= 1;
        }
        return *this;
    }

private:
    char* _head;
    char* _tail;
};


class JSONUI {
public:
    void addMeta(stringstream& ss) {
        if (_meta_count > 0) {
            ss << "\"meta\":[";
            for (size_t i = 0; i < _meta_count; ++i) {
                const auto& key = _meta_keys[i];
                const auto& val = _meta_vals[i];
                ss << "{\"" << key << "\":\"" << val << "\"},";
            }
            ss << remove_comma << "],";
            _meta_count = 0;
        }
    }

    void openTabBox(const char* label) {
        openGenericGroup(label, "tgroup");
    }
    void openHorizontalBox(const char* label) {
        openGenericGroup(label, "hgroup");
    }
    void openVerticalBox(const char* label) {
        openGenericGroup(label, "vgroup");
    }
    void openGenericGroup(const char* label, const char* name) {
        _group_stack[_group_count] = label;
        _group_count += 1;
        _ui_stream << "{";
        _ui_stream << "\"type\":\"" << name << "\",";
        _ui_stream << "\"label\":\"" << label << "\",";
        addMeta(_ui_stream);
        _ui_stream << "\"items\":[";
    }

    void closeBox() {
        _group_count -= 1;
        _ui_stream << remove_comma << "]},";
    }

    void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) {
        addGenericEntry(label, "vslider", zone, init, min, max, step);
    }
    void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) {
        addGenericEntry(label, "hslider", zone, init, min, max, step);
    }
    void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) {
        addGenericEntry(label, "nentry", zone, init, min, max, step);
    }
    void addGenericEntry(const char* label, const char* name, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) {
        _ui_stream << "{\"type\":\"" << name << "\",";
        _ui_stream << "\"label\":\"" << label << "\",";
        _ui_stream << "\"address\":\"";
        for (size_t i = 0; i < _group_count; ++i) {
            const auto& group = _group_stack[i];
            _ui_stream << "/" << group;
        }
        _ui_stream << "/" << label << "\",";
        _ui_stream << "\"index\":" << reinterpret_cast<int>(zone) << ",";
        addMeta(_ui_stream);
        _ui_stream << "\"init\":" << init << ",";
        _ui_stream << "\"min\":" << min << ",";
        _ui_stream << "\"max\":" << max << ",";
        _ui_stream << "\"step\":" << step << "},";
    }

    void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) {
        addGenericBarGraph(label, "hbargraph", zone, min, max);
    }
    void addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) {
        addGenericBarGraph(label, "vbargraph", zone, min, max);
    }
    void addGenericBarGraph(const char* label, const char* name, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) {
        _ui_stream << "{\"type\":\"" << name << "\",";
        _ui_stream << "\"label\":\"" << label << "\",";
        _ui_stream << "\"address\":\"";
        for (size_t i = 0; i < _group_count; ++i) {
            const auto& group = _group_stack[i];
            _ui_stream << "/" << group;
        }
        _ui_stream << "/" << label << "\",";
        _ui_stream << "\"index\":" << reinterpret_cast<int>(zone) << ",";
        _ui_stream << "\"min\":" << min << ",";
        _ui_stream << "\"max\":" << max << "},";
    }

    void declare(FAUSTFLOAT* zone, const char* key, const char* val) {
        _meta_keys[_meta_count] = key;
        _meta_vals[_meta_count] = val;
        _meta_count += 1;
    }

    void declare(const char* key, const char* val) {
        declare(nullptr, key, val);
    }

    void JSON(char* dest, const char* name, const char* filename, int inputs, int outputs, unsigned char* offset) {
        stringstream JSON{dest};
        JSON << "{";
        JSON << "\"name\":\"" << name << "\",";
        JSON << "\"filename\":\"" << filename << "\",";
        JSON << "\"inputs\":" << inputs << ",";
        JSON << "\"outputs\":" << outputs << ",";
        JSON << "\"size\":" << reinterpret_cast<int>(offset) << ",";
        JSON << "\"ui\":[" << _ui_stream << remove_comma << "],";
        addMeta(JSON);
        JSON << remove_comma << "}";
    }

private:
    char _ui_buf[4*4096];
    stringstream _ui_stream{_ui_buf};
    const char* _meta_keys[100];
    const char* _meta_vals[100];
    size_t _meta_count;
    const char* _group_stack[100];
    size_t _group_count;
};

#endif
