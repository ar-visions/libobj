#include <silver/silver.hpp>

/// use mx internally
#include <mx/mx.hpp>

namespace ion {

struct Ident:A {
    vector<str> strings;

    Ident()                     : A(typeof(Ident)) { }
    Ident(const   M &m)         : Ident() {
        if (m.type() == typeof(String))
            strings = strings = vector<str> { str(m) };
        else if (m.type() == typeof(Ident))
            strings = ((Ident*)m.a)->strings;
        else if (m.type() == typeof(Vector<M>))
            for (String &s: *(Vector<M>*)m.a)
                strings += s;
        else if (m.type() == typeof(Vector<str>))
            for (String &s: *(Vector<str>*)m.a)
                strings += s;
    }
    String *to_string() override {
        str res(1024);
        for (String &s: strings) {
            if (res)
                res += ".";
            res += s;
        }
        return res;
    }
    int compare(const M &m) const override {
        bool same;
        if (m.type() == typeof(Ident)) {
            Ident &b = (Ident&)m;
            if (b.strings->len() == strings->len()) {
                int i = 0;
                same = true;
                for (str &s: strings) {
                    if (s != b.strings[i++]) {
                        same = false;
                        break;
                    }
                }
            } else
                same = false;
        } else {
            str b = m;
            if (strings->len() == 1)
                same = strings[0] == b;
            else
                same = false;
        }
        return same ? 0 : -1;
    }
    u64 hash() override {
        u64 h = OFFSET_BASIS;
        for (str &fs: strings) {
            h *= FNV_PRIME;
            h ^= fs->hash();
        }
        return h;
    }
    bool operator==(const str &s) const {
        if (len() == 1 && strings[0] == s)
            return true;
        return false;
    }
    str &operator[](int i) const { return strings[i]; }
    int len()              const { return strings->len(); }
    operator bool()        const { return strings->len() > 0; }
};



static vector<str> keywords = { "method", "import" };

struct ident {
    A_decl(ident, Ident)
    str &operator[](int i) const { return (*data)[i]; }
};

A_impl(ident, Ident)

struct Line {
    vector<ident> tokens;
    int   indent;
    int   num;
    
    int find(str token) {
        int i = 0;
        for (ident &t: tokens) {
            if (t == M(token))
                return i;
            i++;
        }
        return -1; 
    }
    static int indent_count(char *&cur) {
        int count = 0;
        while (*cur == '\t') {
            count++;
            cur++;
        }
        return count;
    }
    static void ws(char *&cur) {
        while (*cur == ' ' || *cur == '\r' || *cur == '\t') ++cur;
    }

    static ident parse_token(char *start, sz_t len) {
        while (start[len - 1] == '\t' || start[len - 1] == ' ')
            len--;
        str all { start, len };
        char t = all[0];
        return (t == '-' || (t >= '0' && t <= '9')) ? Ident(all) : Ident(all->split(".")); /// mr b: im a token!
    }

    static vector<Line> read_tokens(str input) {
        str           sp         = "$()![]/+-*:\"\'#"; /// needs string logic in here to make a token out of the entire "string inner part" without the quotes; those will be tokens neighboring
        char          until      = 0; /// either ) for $(script) ", ', f or i
        sz_t          len        = input->len();
        int           indents    = 0;
        char*         origin     = input->cs();
        char*         start      = 0;
        char*         cur        = origin - 1;
        int           line_num   = 0;
        bool          new_line   = true;
        bool          token_type = false;
        bool          found_null = false;
        vector<Line>  res;
        vector<ident> tokens;
        ///
        while (*(++cur)) {
            if (!until) {
                if (new_line) {
                    indents = indent_count(cur);
                    new_line = false;
                }
                ws(cur);
            }
            if (!*cur) break;
            bool add_str = false;
            char *rel = cur;
            if (*cur == '#') { // comment
                while (*cur && *cur != '\n')
                    cur++;
                found_null = !*cur;
                new_line = true;
            }
            if (until) {
                if (*cur == until && *(cur - 1) != '/') {
                    add_str = true;
                    until = 0;
                    cur++;
                    rel = cur;
                }
            }
            if (!until) {
                int type = sp->index_of(*cur);
                new_line |= *cur == '\n';

                if (start && (add_str || (token_type != (type >= 0) || token_type) || new_line)) {
                    tokens += parse_token(start, (sz_t)(rel - start));
                    if (!add_str) {
                        if (*cur == '$' && *(cur + 1) == '(') // shell
                            until = ')';
                        else if (*cur == '"') // double-quote
                            until = '"';
                        else if (*cur == '\'') // single-quote
                            until = '\'';
                    }
                    if (new_line) {
                        start = null;
                    } else {
                        ws(cur);
                        start = cur;
                        token_type = (type >= 0);
                    }
                }
                else if (!start && !new_line) {
                    start = cur;
                    token_type = (type >= 0);
                }
            }
            if (new_line) {
                if (tokens) {
                    res += Line { .tokens = tokens, .indent = indents, .num = line_num };
                    tokens = {};
                }
                until = 0;
                line_num++;
                if (found_null)
                    break;
            }
        }
        if (tokens)
            res += Line { .tokens = tokens, .indent = indents, .num = line_num };
        return res;
    }
};

struct ENode:A {
    enums(Type, Undefined,
        Undefined, LiteralReal, LiteralInt, LiteralStr, LiteralStrInterp, Array, Var, Add, Sub, Mul, Div, Or, And, Xor, MethodDef, MethodCall, MethodReturn)

    Type    etype;
    M       value;
    vector<ENode> operands;

    static ENode* create_operation(Type etype, const vector<ENode>& operands) {
        ENode* op = new ENode;
        op->etype    = etype;
        op->operands = operands;
        return op;
    }

    static ENode* create_value(Type etype, const M& value) {
        ENode* op = new ENode;
        op->etype    = etype;
        op->value    = value;
        return op;
    }

    static M lookup(const vector<map> &stack, ident id, bool top_only, bool &found) {
        for (int i = stack->len() - 1; i >= 0; i--) {
            map &m = stack[i];
            Field *f = m->fetch(id);
            if (f) {
                found = true;
                return f->value;
            }
            if (top_only)
                break;
        }
        found = false;
        return null;
    }

    static str string_interpolate(const M &m_input, const vector<map> &stack) {
        str input = str(m_input);
        str output = input->interpolate([&](const str &arg) -> str {
            ident f = arg;
            bool found = false;
            M m = lookup(stack, f, false, found);
            if (!found)
                return arg;
            return str(m.to_string());
        });
        return output;
    }
    
    static var exec(const ENode &op, const vector<map> &stack, bool k, int r) {
        switch (op.etype) {
            case Type::LiteralInt:
            case Type::LiteralReal:
            case Type::LiteralStr:
                return var(op.value);
            case Type::LiteralStrInterp:
                return var(string_interpolate(op.value, stack));
            case Type::Array: {
                array res(op.operands->len());
                for (ENode& operand: op.operands) {
                    var v = exec(operand, stack, k, r + 1);
                    res->push(v);
                }
                return var(res);
            }
            case Type::Var: {
                bool is_isolate = (k && r == 0); /// key-mode, and the variable is isolated; that means we dont perform a depth search
                assert(op.value.type() == typeof(Ident));
                bool found = false;
                M m = lookup(stack, op.value, is_isolate, found);
                if (found)
                    return is_isolate ? var(op.value) : var(m); /// in key results, we return its field
                if (is_isolate) {
                    M new_value;
                    Map &top = stack[stack->len() - 1];
                    top[op.value] = new_value;
                    return var(op.value); /// new value is made on stack
                }
                console.fault("var not found from field: {0}", { op.value });
                break;
            }
            case Type::Add: return exec(op.operands[0], stack, k, r + 1) +  exec(op.operands[1], stack, k, r + 1);
            case Type::Sub: return exec(op.operands[0], stack, k, r + 1) -  exec(op.operands[1], stack, k, r + 1);
            case Type::Mul: return exec(op.operands[0], stack, k, r + 1) *  exec(op.operands[1], stack, k, r + 1);
            case Type::Div: return exec(op.operands[0], stack, k, r + 1) /  exec(op.operands[1], stack, k, r + 1);
            case Type::And: return exec(op.operands[0], stack, k, r + 1) && exec(op.operands[1], stack, k, r + 1);
            case Type::Or:  return exec(op.operands[0], stack, k, r + 1) || exec(op.operands[1], stack, k, r + 1);
            case Type::Xor: return exec(op.operands[0], stack, k, r + 1) ^  exec(op.operands[1], stack, k, r + 1);
            default:
                break;
        }
        return var(null);
    }

    operator bool() { return etype != null; }
};

enums(Access, Public,
    Public, Intern)

struct enode {
    A_decl(enode, ENode)
};
A_impl(enode, ENode)



struct EProp:A {
    str             name;
    Access          access;
    vector<ident>   type;
    vector<ident>   value;

    EProp() : A(typeof(EProp)) { }
    operator bool() { return bool(name); }
};

struct eprop {
    A_decl(eprop, EProp)
};
A_impl(eprop, EProp)

struct EnumSymbol:A {
    str             name;
    int             value;
    EnumSymbol() : A(typeof(EnumSymbol)) { }
    operator bool() { return bool(name); }
};

struct enum_symbol {
    A_decl(enum_symbol, EnumSymbol)
};
A_impl(enum_symbol, EnumSymbol)

struct EnumDef:A {
    str             name;
    vector<enum_symbol> symbols;

    EnumDef() : A(typeof(EnumDef)) { }
    operator bool() { return bool(name); }
};

struct enum_def {
    A_decl(enum_def, EnumDef)
};
A_impl(enum_def, EnumDef)


struct EArg:A {
    str             name;
    str             type;
    vector<ident>   default_value;

    EArg() : A(typeof(EArg)) { }
    operator bool() { return bool(name); }
};

struct earg {
    A_decl(earg, EArg)
};
A_impl(earg, EArg)

struct EConstruct:A {
    Access          access;
    vector<earg>    args;
    vector<vector<ident>> initializers;
    vector<ident>   body;

    EConstruct() : A(typeof(EConstruct)) { }
    operator bool() { return bool(body); }
};

struct econstruct {
    A_decl(econstruct, EConstruct)
};
A_impl(econstruct, EConstruct)


struct EMethod:A {
    str             name;
    Access          access;
    vector<earg>    args;
    vector<ident>   body;

    EMethod() : A(typeof(EMethod)) { }
    operator bool() { return bool(name); }
};

struct emethod {
    A_decl(emethod, EMethod)
};
A_impl(emethod, EMethod)

struct EClass:A {
    EClass() : A(typeof(EClass)) { }
    str         name;
    bool        internal;
    bool        is_class; // if false, is struct
    map         econstructs;
    map         eprops;
    map         emethods;
    map         friends; // these friends have to exist in the module
    operator bool() { return bool(name); }
};
struct eclass {
    A_decl(eclass, EClass)
};
A_impl(eclass, EClass)


struct EImport:A {
    EImport() : A(typeof(EImport)) { }
    str         name;
    path        module_path;
    operator bool() { return bool(name); }
};
struct eimport {
    A_decl(eimport, EImport)
};
A_impl(eimport, EImport)


struct EInclude:A {
    EInclude() : A(typeof(EInclude)) { }
    vector<str> library; /// optional libraries
    path        include_file;
    operator bool() { return bool(include_file); }
};
struct einclude {
    A_decl(einclude, EInclude)
};
A_impl(einclude, EInclude)


struct EModule:A {
    EModule() : A(typeof(EModule)) { }
    vector<eimport>  imports;
    vector<einclude> includes;
    map         classes; /// key by name alone, doesnt need to be a full identifier, just a string
    map         data;

    static EModule *parse(path module_path) {
        EModule *m = new EModule();
        str contents = module_path->read<str>();
        vector<Line> lines = Line::read_tokens(contents);

        return m;
    }
};

struct emodule {
    A_decl(emodule, EModule)
};
A_impl(emodule, EModule)

struct Descent {
    vector<type_t> types = { typeof(path) };    
    vector<ident>  tokens;
    int            count;
    int            index;
    
    Descent(vector<ident> &a_tokens, int from, int count) : tokens(count), index(0), count(count) {
        int ii = 0;
        for (int i = from; i < from + count; i++)
            tokens += a_tokens[i];
        tokens += ident {};
    }
    
    bool expect(const ident &token) {
        assert(index < count);
        if (token != tokens[index])
            console.fault("expected token: {0}", {token});
        return true;
    }

    ENode::Type is_numeric() {
        ident token = tokens[index];
        if (token->len() > 1)
            return ENode::Type::Undefined;
        char t = tokens[index][0][0];
        return (t >= '0' && t <= '9') ? (strchr(token[0]->cs(), '.') ? ENode::Type::LiteralReal : ENode::Type::LiteralInt) : ENode::Type::Undefined;
    }

    ENode::Type is_string() {
        ident token = tokens[index];
        if (token->len() > 1)
            return ENode::Type::Undefined;
        char t = tokens[index][0][0];
        return t == '"' ? ENode::Type::LiteralStr : t == '\'' ? ENode::Type::LiteralStrInterp : ENode::Type::Undefined;
    }

    ENode::Type is_var() { /// type, var, or method (all the same); its a name that isnt a keyword
        ident &token = tokens[index];
        char t = token[0][0];
        if (isalpha(t) && keywords->index_of(token[0]) == -1) {
            /// lookup against variable table; declare if in isolation
            return ENode::Type::Var;
        }
        return ENode::Type::Undefined;
    }

    ident &consume(const ident &token) {
        expect(token);
        return tokens[index++];
    }

    ident &consume() {
        assert(index < count);
        return tokens[index++];
    }

    ident &next() {
        return tokens[index];
    }

    enode parse_add() {
        enode left = parse_mult();
        while (next() == "+" || next() == "/") {
            ENode::Type etype = next() == "+" ? ENode::Type::Add : ENode::Type::Sub;
            consume();
            enode right = parse_mult();
            left = ENode::create_operation(etype, { left, right });
        }
        return left;
    }

    enode parse_mult() {
        enode left = parse_primary();
        while (next() == "*" || next() == "/") {
            ENode::Type etype = next() == "*" ? ENode::Type::Mul : ENode::Type::Div;
            consume();
            enode right = parse_primary();
            left = ENode::create_operation(etype, { left, right });
        }
        return left;
    }

    enode parse_primary() {
        ident &id = next();
        console.log("parse_primary: {0}", { id });
        ENode::Type n = is_numeric();
        if (n) {
            ident f = next();
            assert(f->len() == 1);
            cstr cs = f[0]->cs();
            bool is_int = n == ENode::Type::LiteralInt;
            consume();
            return ENode::create_value(n, M::from_string(cs, is_int ? typeof(i64) : typeof(double)));
        }
        ENode::Type s = is_string();
        if (s) {
            ident f = next();
            assert(f->len() == 1);
            cstr cs = f[0]->cs();
            consume();
            str str_literal = M::from_string(cs, typeof(String));
            assert(str_literal->len() >= 2);
            str_literal = str_literal->mid(1, str_literal->len() - 2);
            return ENode::create_value(s, str_literal);
        }
        ENode::Type i = is_var(); /// its a variable or method (same thing; methods consume optional args)
        if (i) {
            ident f = next();
            consume();
            return ENode::create_value(i, f); /// the entire Ident is value
        }

        if (next() == "(") {
            consume();
            enode op = parse_expression();
            expect(str(")"));
            consume();
            return op;
        }
        return {};
    }

    enode parse_expression() {
        return parse_add();
    }

    emodule parse() {

    }
};

struct Expression;
struct expression {
    A_decl(expression, Expression)
};

struct Expression:A {
    enode key;
    enode val;
    str   text;
    vector<expression> children;

    Expression() : A(typeof(Expression)) { }
    
    static enode parse_ops(vector<ident> &tokens, int start, int count) {
        Descent des { tokens, start, count };
        return des.parse();
    }

    static vector<expression> parse(const path& ta) {
        str       contents = ta->read<str>();
        vector<Line> lines = Line::read_tokens(contents);
        vector<expression> res;
        vector<vector<expression>*> parents = { &res };

        str str_path = ta->string();
        ///
        if (!lines) {
            console.fault("tapestry module empty: {0}", array { str_path });
        } else {
            expression  cur;
            expression  prev;
            int         prev_indent = 0;
            int         line_num = 0;
            for (Line &line: lines) {
                int diff = line.indent - prev_indent;
                if (diff > 0) {
                    if (diff != 1) {
                        console.fault("too many indents at {0}:{1}", { ta, line.num });
                        res->clear();
                        break;
                    }
                    parents->push(&cur->children); // this needs to make a M pointer
                } else if (diff < 0) {
                    for (int i = 0; i < -diff; i++)
                        parents->pop();
                }
                cur = expression { };
                *parents[parents->len() - 1] += cur;

                /// read line tokens
                int col         = line.find(":");
                int key_ops     = col == -1 ? line.tokens->len() : col;
                int val_ops     = col == -1 ? 0 : (line.tokens->len() - (col + 1));

                for (ident& id: line.tokens) {
                    str line_str = id->to_string();
                    printf("line: %s\n", line_str->cs());
                }

                assert(key_ops);
                if (key_ops) {
                    ident f = line.tokens[0];
                    cur->key = parse_ops(line.tokens, 0, key_ops);
                }
                if (val_ops)
                    cur->val = parse_ops(line.tokens, key_ops + 1, val_ops);
                prev = cur;
            }
        }
        return res;
    }
    static void exec_expressions(vector<expression> &expr, vector<map> &stack) {
        map top;
        stack->push(top);
        for (expression &e: expr) {
            /// the val needs to exec first, because it may refine a variable
            var val = ENode::exec(e->val, stack, false, 0);
            var key = ENode::exec(e->key, stack, true,  0);
            if (key.type() == typeof(Ident)) { /// set variable to value
                str skey = key.to_string();
                str sval = val.to_string();
                top[key] = val;
                if (e->children)
                    exec_expressions(e->children, stack);
            } else if (key.type() == typeof(Vector<M>)) { /// elements loop
                assert(val.type() == typeof(Ident));
                if (e->children) {
                    vector<M> a(key);
                    for (M &element: a) {
                        top[val] = element; /// each iteration, assign the element to this Ident
                        exec_expressions(e->children, stack);
                    }
                }
            } else if (bool(key)) { /// if statement; a while can be   while: a < 2
                /// a: a + 1
                if (e->children)
                    exec_expressions(e->children, stack);
            }
        }
        //stack.pop<map>();
    }
    static void exec(vector<expression> &expr) {
        vector<map> stack(64);
        exec_expressions(expr, stack);
        for (map &m: stack) {
            for (Field &f: m) {
                console.log("{0}: {1}", { f.key, f.value });
            }
        }
    }
    operator bool() {
        return key || val || text || children;
    }
};

A_impl(expression, Expression)

}

using namespace ion;

int main(int argc, char **argv) {
    map  def     {field { "source",  path(".") }, field { "build", path(".") }, field { "project", str("project") }};
    map  args    { map::args(argc, argv, def) };
    str  project { args["project"] };
    path source  { args["source"]  };
    path build   { args["build"]   };
    path ta      { source / (project + str(".ta")) };
    str  str_path = ta->string();

    if (!ta->exists()) {
        console.fault("tapestry project not found: {0}", { str_path });
        return -1;
    }
    console.log("project is {0}", { str_path });

    /// we should only have 1 mode, which will generate build instructions, then build everytime; it will always check for differences in generation
    /// it could generate ninja too, but i think thats too easy to implement to rely on something like that

    /// the script will have access to our types, and their methods
    vector<expression> e = Expression::parse(ta);
    

    
    //Expression::exec(e);
    return e ? 0 : -1;
}