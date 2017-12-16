#include <obj.h>
#include <prop.h>
#include <primitives.h>

implement(Base)

void Base_class_preinit(class c) { }

void Base_class_init(class c) {
    class_Base cbase = (class_Base)c;
    if (!cbase->meta)
        cbase->meta = new(Pairs);
    Pairs props = new(Pairs);
    pairs_add(cbase->meta, string("props"), props);
    release(props);

    char **mnames = (char **)cbase->mnames;
    cbase->pcount = 0;
    for (int i = 0; i < cbase->mcount; i++) {
        char *start = mnames[i];
        if (strchr(start, '*'))
            continue;
        char *mname = strchr(start, ' ');
        if (mname && strncmp(mname, " get_", 5) == 0)
            cbase->pcount++;
    }
    if (!cbase->pcount)
        return;
    for (int i = 0; i < cbase->mcount; i++) {
        char *start = mnames[i];
        if (strchr(start, '*'))
            continue;
        char *mname = strchr(start, ' ');
        if (mname && strncmp(mname, " get_", 5) == 0) {
            printf("adding property: %s\n", mname);
            mname++;
            char *args = strchr(mname, ' ');
            if (!args)
                continue;
            int type_len = mname - start - 1;
            int name_len = args - mname - 4;
            char type[type_len + 1];
            strncpy(type, start, type_len);
            type[type_len] = 0;
            char name[name_len + 1];
            strncpy(name, &mname[4], name_len);
            name[name_len] = 0;
            Prop p = class_call(Prop, new_with, type, name, (Getter)cbase->m[i], (Setter)cbase->m[i - 1]);
            printf("added property: %s, %s\n", type, name);
            if (p)
                pairs_add(props, string(name), p);
        }
    }
}

void Base_init(Base this) { }

int Base_compare(Base a, Base b) {
    return (long long)a - (long long)b;
}

const char *Base_to_cstring(Base this) {
    String str = self(to_string);
    return str ? str->buffer : NULL;
}

Base Base_from_cstring(const char *value) {
    return NULL;
}

bool Base_is_logging(Base this) {
    return true;
}

void Base_log(Base this, char *format, ...) {
    /*
    va_list args;
    va_start(args, format);

    // scan through format; for each valid formatter % given, call sprintf
    int f_start = -1;
    for (int i = 0, len = strlen(format); i < len; i++) {
        char c = format[i];
        if (c == '%') {
            f_start = (f_start == -1) ? i : -1;
        } else if (f_start != -1) {
            switch (c) {
                case 'i':
                case 'd':
                    switch ()
                    break;

                case 'o':
                    break;

                case 's':
                    break;

                case '':
                    break;
                
                case 'p':
                    break;
            }
        }
            
            // sprintf
        }
        int type = va_arg(args, enum mytypes);
    }
    va_end(args);
    
    va_start(args, format);
    vprintf(format, args);
    */
}

void Base_set_property(Base this, const char *name, Base base_value) {
    Pairs props = pairs_value(this->class->meta, string("props"), Pairs);
    if (!props)
        return;
    Prop p = pairs_value(props, string(name), Prop);
    if (!p)
        return;
    String value = call(base_value, to_string);
    if (!p->enum_type)
        return;
    switch (p->enum_type->ordinal) {
        case Type_Bool: {
            bool v = (value && strcmp(value->buffer, "true") == 0) ? true : false;
            p->setter(this, (void *)(size_t)v);
            break;
        }
        case Type_Int8: {
            char v = (char)atoi(value->buffer);
            p->setter(this, (void *)(size_t)v);
            break;
        }
        case Type_UInt8: {
            unsigned char v = (unsigned char)atoi(value->buffer);
            p->setter(this, (void *)(size_t)v);
            break;
        }
        case Type_Int16: {
            short v = (short)atoi(value->buffer);
            p->setter(this, (void *)(size_t)v);
            break;
        }
        case Type_UInt16: {
            unsigned short v = (unsigned short)atoi(value->buffer);
            p->setter(this, (void *)(size_t)v);
            break;
        }
        case Type_Int32: {
            int v = (int)atoi(value->buffer);
            p->setter(this, (void *)(size_t)v);
            break;
        }
        case Type_UInt32: {
            unsigned int v = (unsigned int)atoi(value->buffer);
            p->setter(this, (void *)(size_t)v);
            break;
        }
        case Type_Int64: {
            long long v = (long long)strtoll(value->buffer, NULL, 10);
            ((void (*)(Base, long long))p->setter)(this, v);
            break;
        }
        case Type_UInt64: {
            unsigned long long v = (unsigned long long)strtoull(value->buffer, NULL, 10);
            ((void (*)(Base, unsigned long long))p->setter)(this, v);
            break;
        }
        case Type_Long: {
            long v = (long)strtoul(value->buffer, NULL, 10);
            p->setter(this, (void *)v);
            break;
        }
        case Type_ULong: {
            unsigned long v = (unsigned long)strtoul(value->buffer, NULL, 10);
            p->setter(this, (void *)v);
            break;
        }
        case Type_Float: {
            float v = (float)atof(value->buffer);
            ((void (*)(Base, float))p->setter)(this, v);
            break;
        }
        case Type_Double: {
            double v = (double)atof(value->buffer);
            ((void (*)(Base, double))p->setter)(this, v);
            break;
        }
        case Type_Object: {
            class_Base c = (class_Base)p->class_type;
            if (c)
                p->setter(this, c->from_string(value));
            break;
        }
        default:
            break;
    }
}

Base Base_get_property(Base this, const char *name) {
    Pairs props = pairs_value(this->class->meta, string("props"), Pairs);
    if (!props)
        return NULL;
    Prop p = pairs_value(props, string(name), Prop);
    if (!p || !p->enum_type)
        return NULL;
    switch (p->enum_type->ordinal) {
        case Type_Object:   return (Base)p->getter(this);
        case Type_Bool:     return (Base)bool_object((bool)((size_t (*)(Base))p->getter)(this));
        case Type_Int8:     return (Base)int8_object(((int8 (*)(Base))p->getter)(this));
        case Type_UInt8:    return (Base)uint8_object(((uint8 (*)(Base))p->getter)(this));
        case Type_Int16:    return (Base)int16_object(((int16 (*)(Base))p->getter)(this));
        case Type_UInt16:   return (Base)uint16_object(((uint16 (*)(Base))p->getter)(this));
        case Type_Int32:
            return (Base)int32_object(((int32 (*)(Base))p->getter)(this));
        case Type_UInt32:   return (Base)uint32_object(((uint32 (*)(Base))p->getter)(this));
        case Type_Int64:    return (Base)int64_object(((int64 (*)(Base))p->getter)(this));
        case Type_UInt64:   return (Base)uint64_object(((uint64 (*)(Base))p->getter)(this));
        case Type_Long:     return (Base)long_object(((long (*)(Base))p->getter)(this));
        case Type_ULong:    return (Base)ulong_object(((ulong (*)(Base))p->getter)(this));
        case Type_Float:    return (Base)float_object(((float (*)(Base))p->getter)(this));
        case Type_Double:   return (Base)double_object(((double (*)(Base))p->getter)(this));
        default:
            break;
    }
    return NULL;
}

Base Base_copy(Base this) {
    Base c = (Base)malloc(this->alloc_size);
    memcpy(c, this, this->alloc_size);
    c->refs = 1;
    return c;
}

Base Base_from_string(String value) {
    if (!value)
        return NULL;
    return class_call(Base, from_cstring, value->buffer);
}

String Base_to_string(Base this) {
    return string("N/A");
}

Base Base_retain(Base this) {
    if (this->refs++ == 0) {
        // remove from ar
        AutoRelease ar = AutoRelease_cl->current();
        call(ar, remove, this);
    }
    return this;
}

void Base_release(Base this) {
    if (this->refs-- == 0) {
        AutoRelease ar = AutoRelease_cl->current();
        call(ar, remove, this);
    }
    if (this->refs <= 0)
        free_obj(this);
}

Base Base_autorelease(Base this) {
    AutoRelease ar = AutoRelease_cl->current();
    if (ar && !this->ar_node) {
        this->refs = 1;
        call(ar, add, this);
    }
    return this;
}

void Base_free(Base this) {
    free(this);
}

ulong Base_hash(Base this) {
    return 0;
}
