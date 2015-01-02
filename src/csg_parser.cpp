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

#include "utils/support.h"
#include "utils/string_switch.h"

#include "csg.h"
#include "voxel.h"

#include <fstream>
#include <map>
#include <cctype>
#include <functional>
#include <sstream>


namespace ocmesh {
namespace details {
    
    using namespace utils::literals;
    
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
            lbrace,
            rbrace,
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
        
        token(kind_t kind, std::string text = std::string{})
            : _kind(kind), _text(std::move(text)) { }
        
        explicit token(float value)
            : _kind(number), _text(std::to_string(value)), _value(value) { }
        
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
    
    token::kind_t find_keyword(std::string const&s)
    {
        switch (utils::str_switch(s)) {
            case "object"_match:
                return token::object;
            case "material"_match:
                return token::material;
            case "build"_match:
                return token::build;
            case "sphere"_match:
            case "cube"_match:
                return token::primitive;
            case "unite"_match:
            case "subtract"_match:
            case "intersect"_match:
                return token::binary;
            case "scale"_match:
            case "xscale"_match:
            case "yscale"_match:
            case "zscale"_match:
            case "rotate"_match:
            case "xrotate"_match:
            case "yrotate"_match:
            case "zrotate"_match:
            case "translate"_match:
            case "xtranslate"_match:
            case "ytranslate"_match:
            case "ztranslate"_match:
                return token::transform;
            default:
                return token::identifier;
        }
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
                return token(token::lparen, "(");
            case ')':
                stream.get();
                return token(token::rparen, ")");
            case '{':
                stream.get();
                return token(token::lbrace, "{");
            case '}':
                stream.get();
                return token(token::rbrace, "}");
            case ';':
                stream.get();
                return token(token::semicolon, ";");
            case ',':
                stream.get();
                return token(token::comma, ",");
            case '=':
                stream.get();
                return token(token::equals, "=");
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
        scene *_scene;
        std::istream &_stream;
        
        token _current;
        std::map<std::string, object *> _bindings;
        std::map<std::string, voxel::material_t> _materials;
        voxel::material_t _last_material = voxel::void_material;
        
    public:
        parser(scene *scene, std::istream &stream)
            : _scene(scene), _stream(stream) { }
        
        void parse()
        {
            while(lex().kind() != token::eof)
                parse_statement();
        }
        
    private:
        token lex() {
            return _current = details::lex(_stream);
        }
        
        template<typename ...Args, REQUIRES(sizeof...(Args) > 0)>
        token lex(Args ...args) {
            lex();
            if(! _current.is(args...)) {
                error("Syntax error: unexpected token '", _current.text(), "'");
            }
            return _current;
        }
        
        void parse_statement()
        {
            switch(_current.kind()) {
                case token::object:
                    return parse_object();
                case token::material:
                    return parse_material();
                case token::build:
                    return parse_build_directive();
                default:
                    code_unreachable();
            }
        }
        
        void parse_object() {
            assert(_current.kind() == token::object);
            
            std::string name = lex(token::identifier).text();

            lex(token::equals);
            
            _bindings[name] = parse_object_expression();
        }
        
        object *parse_object_expression()
        {
            lex(token::identifier, token::primitive,
                token::binary, token::transform);
            
            switch (_current.kind()) {
                case token::identifier:
                    return parse_variable_reference();
                case token::primitive:
                    return parse_primitive();
                case token::binary:
                    return parse_binary();
                case token::transform:
                    return parse_transform();
                default:
                    code_unreachable();
            }
        }
        
        object *parse_variable_reference() {
            assert(_current.is(token::identifier));
            
            std::string name = _current.text();
            
            if(_bindings.find(name) == _bindings.end())
                error("Use of undeclared object identifier '", name, "'");
            
            return _bindings[name];
        }
        
        object *parse_primitive()
        {
            assert(_current.is(token::primitive));
            
            std::string name = _current.text();
            lex(token::lparen);
            float argument = lex(token::number).value();
            lex(token::rparen);
            
            switch (utils::str_switch(name)) {
                case "cube"_match:
                    return _scene->cube(argument);
                case "sphere"_match:
                    return _scene->sphere(argument);
                default:
                    code_unreachable();
            }
        }
        
        object *parse_binary()
        {
            assert(_current.is(token::binary));
            
            std::string name = _current.text();
            
            lex(token::lparen);
            object *lhs = parse_object_expression();
            lex(token::comma);
            object *rhs = parse_object_expression();
            lex(token::rparen);
            
            switch (utils::str_switch(name)) {
                case "unite"_match:
                    return csg::unite(lhs, rhs);
                case "intersect"_match:
                    return csg::intersect(lhs, rhs);
                case "subtract"_match:
                    return csg::subtract(lhs, rhs);
                default:
                    code_unreachable();
            }
        }
        
        void parse_material() {
            assert(_current.is(token::material));

            lex(token::identifier);
            _materials[_current.text()] = ++_last_material;
        }
        
        void parse_build_directive() {
            assert(_current.is(token::build));
            
            std::string name = lex(token::identifier).text();
            std::string material = lex(token::identifier).text();
            
            if(_bindings.find(name) == _bindings.end())
                error("Use of undeclared object identifier '", name, "'");
            if(_materials.find(material) == _materials.end())
                error("Use of undeclared material identifier '", name, "'");
            
            object *obj = _bindings[name];
            obj->scene()->toplevel(obj, _materials[material]);
        }
        
        object *parse_transform() {
            assert(_current.is(token::transform));
            
            switch(utils::str_switch(_current.text())) {
                case "scale"_match:
                    return parse_scale();
                case "rotate"_match:
                    return parse_rotate();
                case "translate"_match:
                    return parse_translate();
                default:
                    return parse_single_component_transform();
            }
        }
        
        glm::vec3 parse_3d_vector(bool consume_brace)
        {
            if(consume_brace)
                lex(token::lbrace);
            
            assert(_current.is(token::lbrace));
            
            glm::vec3 v;

            v.x = lex(token::number).value();
            lex(token::comma);
            
            v.y = lex(token::number).value();
            lex(token::comma);
            
            v.z = lex(token::number).value();
            lex(token::rbrace);
            
            return v;
        }
        
        object *parse_scale()
        {
            lex(token::lparen);
            lex(token::lbrace, token::number);
            
            switch (_current.kind()) {
                case token::number: {
                    float f = _current.value();
                    lex(token::comma);
                    object *obj = parse_object_expression();
                    lex(token::rparen);

                    return csg::scale(obj, f);
                }
                case token::lbrace: {
                    glm::vec3 factors = parse_3d_vector(false);
                    lex(token::comma);
                    object *obj = parse_object_expression();
                    lex(token::rparen);
                    
                    return csg::scale(obj, factors);
                }
                default:
                    code_unreachable();
            }
        }
        
        object *parse_rotate() {
            lex(token::lparen);
            
            float angle = lex(token::number).value();
            lex(token::comma);
            
            glm::vec3 axis = parse_3d_vector(true);
            lex(token::comma);
            
            object *obj = parse_object_expression();
            lex(token::rparen);
            
            return csg::rotate(obj, angle, axis);
        }
        
        object *parse_translate() {
            lex(token::lparen);
            
            glm::vec3 offsets = parse_3d_vector(true);
            lex(token::comma);
            object *obj = parse_object_expression();
            lex(token::rparen);
            
            return csg::translate(obj, offsets);
        }
        
        object *parse_single_component_transform() {
            assert(_current.is(token::transform));
            
            lex(token::lparen);
            float argument = lex(token::number).value();
            lex(token::comma);
            object *obj = parse_object_expression();
            lex(token::rparen);
            
            switch (utils::str_switch(_current.text())) {
                case "xscale"_match:
                    return csg::xscale(obj, argument);
                case "yscale"_match:
                    return csg::yscale(obj, argument);
                case "zscale"_match:
                    return csg::zscale(obj, argument);
                case "xrotate"_match:
                    return csg::xrotate(obj, argument);
                case "yrotate"_match:
                    return csg::yrotate(obj, argument);
                case "zrotate"_match:
                    return csg::zrotate(obj, argument);
                case "xtranslate"_match:
                    return csg::xtranslate(obj, argument);
                case "ytranslate"_match:
                    return csg::ytranslate(obj, argument);
                case "ztranslate"_match:
                    return csg::ztranslate(obj, argument);
                default:
                    code_unreachable();
            }
        }
        
        void concatenate(std::ostream &) { }
        
        template<typename T, typename ...Args>
        void concatenate(std::ostream &s, T&& v, Args&& ...args) {
            s << std::forward<T>(v);
            return concatenate(s, std::forward<Args>(args)...);
        }
        
        template<typename ...Args>
        [[noreturn]]
        void error(Args&& ...args)
        {
            std::stringstream str;
            
            concatenate(str, std::forward<Args>(args)...);
            
            throw scene::parse_result(false, str.str());
        }
    };
    
    scene::parse_result scene::parse(std::istream &stream)
    {
        try {
            parser(this, stream).parse();
        } catch(scene::parse_result r) {
            return r;
        }
        
        return { };
    }
    
} // namespace details
} // namespace ocmesh
