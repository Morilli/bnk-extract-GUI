#ifndef _ERRORS_H
#define _ERRORS_H

#include <cstdio>
#include <string>
using namespace std;

class Argument_error {
    string errmsg;
public:
    inline void print(FILE* out) const {
        fprintf(out, "Argument error: %s", errmsg.c_str());
    }

    explicit inline Argument_error(const char * str) : errmsg(str) {}
};

class File_open_error {
    string filename;
public:
    inline void print(FILE* out) const {
        fprintf(out, "Error opening: %s", filename.c_str());
    }

    explicit inline File_open_error(const string& name) : filename(name) {}
};

class Parse_error {
protected:
    virtual inline void print_self(FILE* out) const {
        fprintf(out, "unspecified.");
    }
public:
    inline void print(FILE* out) const {
        fprintf(out, "Parse error: ");
        print_self(out);
        fprintf(out, "\n");
    }
    inline virtual ~Parse_error() = default;
};

class Parse_error_str : public Parse_error {
    string str;
protected:
    inline void print_self(FILE* out) const override {
        fprintf(out, "%s", str.c_str());
    }
public:
    explicit inline Parse_error_str(string s) : str(s) {}
};

class Size_mismatch : public Parse_error {
    const unsigned long long real_size, read_size;
protected:
    inline void print_self(FILE* out) const override {
        fprintf(out, "expected %llu bits, read %llu", real_size, read_size);
    }
public:
    explicit inline Size_mismatch(unsigned long real_s, unsigned long read_s) : real_size(real_s), read_size(read_s) {}
};

class Invalid_id : public Parse_error {
    const int id;
protected:
    inline void print_self(FILE* out) const override {
        fprintf(out, "invalid codebook id %d, try --inline-codebooks", id);
    }
public:
    explicit inline Invalid_id(int i) : id(i) {}
};


#endif
