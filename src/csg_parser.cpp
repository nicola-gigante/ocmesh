/*
 * Copyright 2014 Nicola Gigante
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "csg.h"
#include "voxel.h"

#include <fstream>
#include <map>
#include <cctype>
#include <functional>


namespace ocmesh {
namespace details {
    
    class token
    {
    public:
        enum kind_t {
            // Special tokens
            unknown = 0,
            eof,
            // Numbers and identifiers
            number,
            identifier,
            // Punctuation
            lparen,
            rparen,
            semicolon,
            comma,
            equals,
            // Keywords
            object,
            material,
            build,
            // Geometric primitives
            primitive,
            binary,
            transform
        };
        
        token() = default;
        
        token(kind_t kind, std::string text)
            : _kind(kind), _text(std::move(text)) { }
        
        token(float value) : _kind(number), _value(value) { }
        
        kind_t kind() const { return _kind; }
        
        std::string text() const { return _text; }
        float value() const { return _value; }
        
        bool is(kind_t k) const {
            return _kind == k;
        }
        
        template<typename ...Args>
        bool is(kind_t k, Args ...args) const {
            return _kind == k || is(args...);
        }
        
    private:
        kind_t _kind = unknown;

        std::string _text;
        float _value = 0;
    };
    
    // Skip whitespaces and comments
    template<typename F>
    void skipwhile(std::istream &stream, F pred) {
        while(pred(stream.peek()))
            stream.get();
    }
    
    void skipcomments(std::istream &stream) {
        while(stream.peek() == '#') {
            skipwhile(stream, [](char c) { return c != '\n'; });
            skipwhile(stream, isspace);
        }
    }
    
    token::kind_t find_keyword(std::string const&s) {
        static const std::map<std::string, token::kind_t> keywords = {
            { "object"    , token::object    },
            { "material"  , token::material  },
            { "build"     , token::build     },
            
            { "sphere"    , token::primitive },
            { "cube"      , token::primitive },
            
            { "unite"     , token::binary    },
            { "intersect" , token::binary    },
            { "subtract"  , token::binary    },
            
            { "transform" , token::transform },
            { "scale"     , token::transform },
            { "xscale"    , token::transform },
            { "yscale"    , token::transform },
            { "zscale"    , token::transform },
            { "rotate"    , token::transform },
            { "xrotate"   , token::transform },
            { "yrotate"   , token::transform },
            { "zrotate"   , token::transform },
            { "translate" , token::transform },
            { "xtranslate", token::transform },
            { "ytranslate", token::transform },
            { "ztranslate", token::transform }
        };
        
        auto it = keywords.find(s);
        if(it != keywords.end())
            return it->second;
        
        return token::identifier;
    }
    
    token lex(std::istream &stream)
    {
        skipwhile(stream, isspace);
        
        skipcomments(stream);
        
        if(stream.eof())
            return token::eof;
        
        // Peek at next character
        char c = stream.peek();
        
        // Lex punctuation
        switch (c) {
            case '(':
                stream.get();
                return token::lparen;
            case ')':
                stream.get();
                return token::rparen;
            case ';':
                stream.get();
                return token::semicolon;
            case ',':
                stream.get();
                return token::comma;
            case '=':
                stream.get();
                return token::equals;
        }
        
        // Lex numbers
        if(c == '-' || std::isdigit(c)) {
            float value;
            stream >> value;
            return token(value);
        }
        
        // Lex identifiers
        if(c == '_' || std::isalpha(c)) {
            std::string id;
            while (c == '_' || std::isalnum(c)) {
                id.push_back(c);
                stream.get();
                c = stream.peek();
            }
            
            return token(find_keyword(id), id);
        }
        
        return token::unknown;
    }
    
    class parser
    {
    public:
        parser(std::istream &stream) : _stream(stream) { }
        
        scene &results() { return _scene; }
        
        void parse()
        {
            while(lex().kind() != token::eof)
                parseStatement();
        }
        
    private:
        token lex() {
            return _current = details::lex(_stream);
        }
        
        template<typename ...Args, REQUIRES(sizeof...(Args) > 0)>
        token lex(Args ...args) {
            lex();
            if(! _current.is(args...))
                error();
            return _current;
        }
        
        void parseStatement() {
            switch(_current.kind()) {
                case token::object:
                    return parseObject();
                case token::material:
                    return parseMaterial();
                case token::build:
                    return parseBuildDirective();
                default:
                    error();
            }
        }
        
        void parseObject() {
            assert(_current.kind() == token::object);
            
            std::string name = lex(token::identifier).text();

            lex(token::equals);
            
            lex(token::primitive, token::binary, token::transform);

            _bindings[name] = parseObjectExpression();
        }
        
        object *parseObjectExpression()
        {
            assert(_current.is(token::primitive, token::binary, token::transform));
            
            switch (_current.kind()) {
                case token::identifier:
                    return parseVariableReference();
                case token::primitive:
                    return parsePrimitive();
                case token::binary:
                    return parseBinary();
                case token::transform:
                    return { };
                default:
                    assert(false && "Unreachable");
                    break;
            }
        }
        
        object *parseVariableReference() {
            assert(_current.is(token::identifier));
            
            std::string name = _current.text();
            
            if(_bindings.find(name) == _bindings.end())
                error();
            
            return _bindings[name];
        }
        
        object *parsePrimitive()
        {
            static const
            std::map<std::string, std::function<object *(float)>> primitives = {
                { "cube"  , [this](float x) { return _scene.sphere(x); } },
                { "sphere", [this](float x) { return _scene.sphere(x); } }
            };
            
            assert(_current.is(token::primitive));
            
            std::string name = _current.text();
            lex(token::lparen);
            float argument = lex(token::number).value();
            lex(token::rparen);
            
            assert(primitives.find(name) != primitives.end());
            
            return primitives.at(name)(argument);
        }
        
        object *parseBinary()
        {
            static const
            std::map<std::string, object *(*)(object *, object *)> binaries = {
                { "unite"    , &csg::unite     },
                { "intersect", &csg::intersect },
                { "subtract" , &csg::subtract  }
            };
            
            assert(_current.is(token::binary));
            
            std::string name = _current.text();
            
            lex(token::lparen);
            object *lhs = parseObjectExpression();
            lex(token::comma);
            object *rhs = parseObjectExpression();
            lex(token::rparen);
            
            assert(binaries.find(name) != binaries.end());
            
            return binaries.at(name)(std::move(lhs), std::move(rhs));
        }
        
        void parseMaterial() {
            assert(_current.is(token::material));

            lex(token::identifier);
            _materials[_current.text()] = ++_last_material;
        }
        
        void parseBuildDirective() {
            assert(_current.is(token::build));
            
            std::string name = lex(token::identifier).text();
            std::string material = lex(token::identifier).text();
            
            if(_bindings.find(name) == _bindings.end())
                error();
            if(_materials.find(material) == _materials.end())
                error();
            
            object *obj = _bindings[name];
            obj->scene()->toplevel(obj, _materials[material]);
        }
        
        void error() {
            assert("Error handling unimplemented");
        }
        
    private:
        std::istream &_stream;
        token _current;
        scene _scene;
        std::map<std::string, object *> _bindings;
        std::map<std::string, voxel::material_t> _materials;
        voxel::material_t _last_material = 0;
    };
    
    std::vector<object *> parse(std::istream &)
    {
        return { };
    }
    
} // namespace details
} // namespace ocmesh
