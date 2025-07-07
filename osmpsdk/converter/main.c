#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 1024
#define MAX_GROUPS 128
#define MAX_REGIONS 256
#define MAX_KV 128

typedef struct {
    char key[64];
    char value[128];
} KeyValue;

typedef struct {
    KeyValue kv[MAX_KV];
    int kv_count;
} Region;

typedef struct {
    char name[128];
    KeyValue kv[MAX_KV];
    int kv_count;
    Region regions[MAX_REGIONS];
    int region_count;
} Group;

typedef struct {
    KeyValue kv[MAX_KV];
    int kv_count;
    Group groups[MAX_GROUPS];
    int group_count;
} SfzYaml;

void trim(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end+1) = 0;
}

void parse_kv_line(const char *line, char *key, char *value) {
    const char *eq = strchr(line, '=');
    if (!eq) {
        key[0] = 0;
        value[0] = 0;
        return;
    }
    size_t klen = eq - line;
    strncpy(key, line, klen);
    key[klen] = 0;
    strcpy(value, eq + 1);
    trim(key);
    trim(value);
    // Lowercase and replace '-' with '_'
    for (char *p = key; *p; ++p) {
        if (*p == '-') *p = '_';
        else *p = tolower(*p);
    }
}

void print_yaml(const SfzYaml *data) {
    printf("global:\n");
    for (int i = 0; i < data->kv_count; ++i) {
        printf("  %s: %s\n", data->kv[i].key, data->kv[i].value);
    }
    printf("groups:\n");
    for (int g = 0; g < data->group_count; ++g) {
        const Group *grp = &data->groups[g];
        printf("  - name: \"%s\"\n", grp->name[0] ? grp->name : "Group");
        for (int k = 0; k < grp->kv_count; ++k) {
            printf("    %s: %s\n", grp->kv[k].key, grp->kv[k].value);
        }
        printf("    regions:\n");
        for (int r = 0; r < grp->region_count; ++r) {
            printf("      -\n");
            for (int k = 0; k < grp->regions[r].kv_count; ++k) {
                printf("        %s: %s\n", grp->regions[r].kv[k].key, grp->regions[r].kv[k].value);
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s input.sfz\n", argv[0]);
        return 1;
    }
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen");
        return 1;
    }

    SfzYaml data = {0};
    Group *current_group = NULL;
    Region *current_region = NULL;
    enum { SEC_NONE, SEC_GLOBAL, SEC_GROUP, SEC_REGION } section = SEC_NONE;

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (line[0] == 0 || strncmp(line, "//", 2) == 0 || line[0] == '#')
            continue;
        if (strncasecmp(line, "<global>", 8) == 0) {
            section = SEC_GLOBAL;
            continue;
        } else if (strncasecmp(line, "<group>", 7) == 0) {
            section = SEC_GROUP;
            if (data.group_count < MAX_GROUPS) {
                current_group = &data.groups[data.group_count++];
                memset(current_group, 0, sizeof(Group));
                current_region = NULL;
            }
            continue;
        } else if (strncasecmp(line, "<region>", 8) == 0) {
            section = SEC_REGION;
            if (!current_group) {
                if (data.group_count < MAX_GROUPS) {
                    current_group = &data.groups[data.group_count++];
                    memset(current_group, 0, sizeof(Group));
                }
            }
            if (current_group && current_group->region_count < MAX_REGIONS) {
                current_region = &current_group->regions[current_group->region_count++];
                memset(current_region, 0, sizeof(Region));
            }
            continue;
        }
        char key[64], value[128];
        parse_kv_line(line, key, value);
        if (!key[0]) continue;
        if (section == SEC_GROUP && current_group) {
            if (strcmp(key, "trigger") == 0 || strcmp(key, "group_name") == 0 || strcmp(key, "name") == 0) {
                strncpy(current_group->name, value, sizeof(current_group->name)-1);
            } else if (current_group->kv_count < MAX_KV) {
                strncpy(current_group->kv[current_group->kv_count].key, key, 63);
                strncpy(current_group->kv[current_group->kv_count].value, value, 127);
                current_group->kv_count++;
            }
        } else if (section == SEC_REGION && current_region) {
            if (current_region->kv_count < MAX_KV) {
                strncpy(current_region->kv[current_region->kv_count].key, key, 63);
                strncpy(current_region->kv[current_region->kv_count].value, value, 127);
                current_region->kv_count++;
            }
        } else if (section == SEC_GLOBAL) {
            if (data.kv_count < MAX_KV) {
                strncpy(data.kv[data.kv_count].key, key, 63);
                strncpy(data.kv[data.kv_count].value, value, 127);
                data.kv_count++;
            }
        }
    }
    fclose(f);

    // Assign default group names if missing
    for (int i = 0; i < data.group_count; ++i) {
        if (!data.groups[i].name[0]) {
            snprintf(data.groups[i].name, sizeof(data.groups[i].name), "Group%d", i+1);
        }
    }

    print_yaml(&data);
    return 0;
}
