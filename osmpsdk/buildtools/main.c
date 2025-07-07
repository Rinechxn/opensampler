#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>
#include <yaml.h>
#include <sys/stat.h>

#define XOR_KEY_DEFAULT 0x5A

// Helper: Read file into buffer
uint8_t* read_file(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* buf = malloc(sz);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    fclose(f);
    *out_size = sz;
    return buf;
}

// Helper: Write little-endian uint16 string
void write_le16str(FILE* f, const char* s) {
    uint16_t len = (uint16_t)strlen(s);
    fwrite(&len, 2, 1, f);
    fwrite(s, 1, len, f);
}

// Helper: XOR encrypt buffer in-place
void xor_encrypt(uint8_t* data, size_t len, uint8_t key) {
    for (size_t i = 0; i < len; ++i) data[i] ^= key;
}

// Helper: Compress buffer with zlib
uint8_t* zlib_compress(const uint8_t* data, size_t in_size, size_t* out_size) {
    uLongf dest_len = compressBound(in_size);
    uint8_t* dest = malloc(dest_len);
    if (!dest) return NULL;
    if (compress(dest, &dest_len, data, in_size) != Z_OK) {
        free(dest); return NULL;
    }
    *out_size = dest_len;
    return dest;
}

// Minimal YAML parsing helpers (for demo only, not robust)
char* get_yaml_value(const char* filename, const char* key) {
    FILE* f = fopen(filename, "r");
    if (!f) return NULL;
    char line[512];
    char* result = NULL;
    while (fgets(line, sizeof(line), f)) {
        char* pos = strstr(line, key);
        if (pos && pos == line) {
            char* colon = strchr(line, ':');
            if (colon) {
                result = strdup(colon + 1);
                char* nl = strchr(result, '\n');
                if (nl) *nl = 0;
                while (*result == ' ' || *result == '\t') ++result;
                break;
            }
        }
    }
    fclose(f);
    return result;
}

#if defined(_WIN32) || defined(_WIN64)
    #define OSMP_PATH_SEP '\\'
    #include <windows.h>
#else
    #define OSMP_PATH_SEP '/'
#endif

// Helper: Join two paths with platform separator
void join_path(char* out, size_t outsz, const char* a, const char* b) {
    size_t la = strlen(a);
    snprintf(out, outsz, "%s%c%s", a, OSMP_PATH_SEP, b);
}

// Main build function (simplified, only 1 group/region/sample for demo)
int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <input_folder> <output_file> [--key 0xNN]\n", argv[0]);
        return 1;
    }
    const char* input_folder = argv[1];
    const char* output_file = argv[2];
    uint8_t xor_key = XOR_KEY_DEFAULT;
    if (argc >= 5 && strcmp(argv[3], "--key") == 0) {
        xor_key = (uint8_t)strtol(argv[4], NULL, 0);
    }

    // Paths
    char meta_path[512], map_path[512], sample_path[512];
    join_path(meta_path, sizeof(meta_path), input_folder, "metadata.yaml");
    join_path(map_path, sizeof(map_path), input_folder, "mapping.yaml");

    // For demo: get sample file from mapping.yaml (assume: sample: "samples/foo.wav")
    char* sample_rel = get_yaml_value(map_path, "sample");
    if (!sample_rel) {
        printf("Sample path not found in mapping.yaml\n");
        return 1;
    }
    join_path(sample_path, sizeof(sample_path), input_folder, sample_rel);

    // Read sample file
    size_t sample_size = 0;
    uint8_t* sample_buf = read_file(sample_path, &sample_size);
    if (!sample_buf) {
        printf("Sample file not found: %s\n", sample_path);
        return 1;
    }

    // Compress and encrypt
    size_t comp_size = 0;
    uint8_t* comp_buf = zlib_compress(sample_buf, sample_size, &comp_size);
    if (!comp_buf) {
        printf("Compression failed\n");
        free(sample_buf);
        return 1;
    }
    xor_encrypt(comp_buf, comp_size, xor_key);

    // Write output file
    FILE* out = fopen(output_file, "wb");
    if (!out) { printf("Cannot open output file\n"); return 1; }
    fwrite("OSMP", 1, 4, out); // magic
    uint32_t version = 1;
    fwrite(&version, 4, 1, out);

    // Write dummy metadata.yaml as string
    size_t meta_sz = 0;
    uint8_t* meta_buf = read_file(meta_path, &meta_sz);
    uint32_t meta_len = (uint32_t)meta_sz;
    fwrite(&meta_len, 4, 1, out);
    fwrite(meta_buf, 1, meta_sz, out);

    // Write dummy global info
    float global_volume = 1.0f;
    uint32_t amp_veltrack = 0;
    fwrite(&global_volume, 4, 1, out);
    fwrite(&amp_veltrack, 4, 1, out);

    // Write 1 group
    uint32_t group_count = 1;
    fwrite(&group_count, 4, 1, out);
    write_le16str(out, "Group");
    uint32_t region_count = 1;
    fwrite(&region_count, 4, 1, out);

    // Write 1 region (dummy values)
    uint8_t lokey=0, hikey=127, lovel=0, hivel=127, pitch_keycenter=60;
    float volume=1.0f, tune=0.0f;
    fwrite(&lokey, 1, 1, out); fwrite(&hikey, 1, 1, out);
    fwrite(&lovel, 1, 1, out); fwrite(&hivel, 1, 1, out);
    fwrite(&pitch_keycenter, 1, 1, out);
    fwrite(&volume, 4, 1, out); fwrite(&tune, 4, 1, out);

    // Write sample offset placeholder
    uint64_t sample_offset = 0;
    uint32_t orig_size = (uint32_t)sample_size;
    uint32_t comp_size32 = (uint32_t)comp_size;
    long offset_pos = ftell(out);
    fwrite(&sample_offset, 8, 1, out);
    fwrite(&orig_size, 4, 1, out);
    fwrite(&comp_size32, 4, 1, out);

    // Write sample data
    long sample_data_offset = ftell(out);
    fwrite(comp_buf, 1, comp_size, out);

    // Patch offset
    fseek(out, offset_pos, SEEK_SET);
    uint64_t sample_data_offset64 = (uint64_t)sample_data_offset;
    fwrite(&sample_data_offset64, 8, 1, out);

    fclose(out);
    free(sample_buf); free(comp_buf); free(meta_buf);
    printf("✅ Build complete! Output saved to: %s\n", output_file);
    printf("🔐 Encryption key: 0x%02X\n", xor_key);
    return 0;
}
