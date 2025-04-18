#include "ValueRefParser.h"

#include "Parse.h"
#include "ConditionParserImpl.h"
#include "EnumValueRefRules.h"
#include "MovableEnvelope.h"
#include "../universe/ValueRefs.h"

#include <boost/phoenix.hpp>

namespace parse {
    string_parser_grammar::string_parser_grammar(
        const parse::lexer& tok,
        detail::Labeller& label,
        const detail::condition_parser_grammar& condition_parser
    ) :
        string_parser_grammar::base_type(expr, "string_parser_grammar"),
        string_var_complex(tok, label, *this)
    {
        namespace phoenix = boost::phoenix;
        namespace qi = boost::spirit::qi;

        using phoenix::new_;
        using phoenix::push_back;

        qi::_1_type _1;
        qi::_2_type _2;
        qi::_3_type _3;
        qi::_4_type _4;
        qi::_a_type _a;
        qi::_b_type _b;
        qi::_c_type _c;
        qi::_d_type _d;
        qi::_val_type _val;
        qi::lit_type lit;
        qi::_pass_type _pass;
        const boost::phoenix::function<detail::construct_movable> construct_movable_;
        const boost::phoenix::function<detail::deconstruct_movable> deconstruct_movable_;
        const boost::phoenix::function<parse::detail::deconstruct_movable_vector> deconstruct_movable_vector_;

        const std::string TOK_CURRENT_CONTENT{ValueRef::Constant<std::string>::current_content};

        bound_variable_name
            %=   tok.Name_
            |    tok.Species_
            |    tok.BuildingType_
            |    tok.FieldType_
            |    tok.Focus_
            |    tok.DefaultFocus_
            |    tok.Hull_
            ;

        constant
            =   tok.string          [ _val = construct_movable_(new_<ValueRef::Constant<std::string>>(_1)) ]
            |  (    tok.CurrentContent_
                |   tok.ThisBuilding_
                |   tok.ThisField_
                |   tok.ThisHull_
                |   tok.ThisPart_   // various aliases for this reference in scripts, allowing scripter to use their preference
                |   tok.ThisPolicy_
                |   tok.ThisTech_
                |   tok.ThisSpecies_
                |   tok.ThisSpecial_
               ) [ _val = construct_movable_(new_<ValueRef::Constant<std::string>>(TOK_CURRENT_CONTENT)) ]
            ;

        free_variable
            =   tok.Value_          [ _val = construct_movable_(new_<ValueRef::Variable<std::string>>(ValueRef::ReferenceType::EFFECT_TARGET_VALUE_REFERENCE)) ]
            |   tok.GalaxySeed_     [ _val = construct_movable_(new_<ValueRef::Variable<std::string>>(ValueRef::ReferenceType::NON_OBJECT_REFERENCE, _1)) ]
            ;

        named_string_valueref
            = (   (tok.Named_ >> tok.String_ >> label(tok.name_))
                 > tok.string > label(tok.value_) > expr
              ) [
                 // Register the value ref under the given name by lazy invoking RegisterValueRef
                 parse::detail::open_and_register_as_string_(_2, _3, _pass),

                 _val = construct_movable_(new_<ValueRef::NamedRef<std::string>>(_2))
              ] | ((tok.Named_ >> tok.String_ >> tok.Lookup_)
                 >  label(tok.name_) > tok.string
              ) [
                 _val = construct_movable_(new_<ValueRef::NamedRef<std::string>>(_2, phoenix::val(/*is_lookup_only*/true)))
              ]
            ;

        named_lookup_expr
          =   (
                   tok.Named_ >> tok.String_ >> tok.Lookup_
                >> label(tok.name_)
                >> tok.string
              ) [ _val = construct_movable_(new_<ValueRef::NamedRef<std::string>>(_4)) ]
            ;

        variable_scope_rule = detail::variable_scope(tok);
        container_type_rule = detail::container(tok);
        detail::initialize_bound_variable_parser<std::string>(
            bound_variable, unwrapped_bound_variable, value_wrapped_bound_variable,
            bound_variable_name, variable_scope_rule, container_type_rule,
            tok);

        statistic_sub_value_ref
            =   constant
            |   bound_variable
            |   free_variable
            |   string_var_complex
            ;

        function_expr
            =   (
                (
                    (
                        tok.OneOf_  [ _c = ValueRef::OpType::RANDOM_PICK ]
                    |   tok.Min_    [ _c = ValueRef::OpType::MINIMUM ]
                    |   tok.Max_    [ _c = ValueRef::OpType::MAXIMUM ]
                    )
                    > (     '('  >   expr [ push_back(_d, _1) ]
                        > *(','  >   expr [ push_back(_d, _1) ] ) > ')' )
                    [ _val = construct_movable_(new_<ValueRef::Operation<std::string>>(_c, deconstruct_movable_vector_(_d, _pass))) ]
                )
                |   (
                    tok.UserString_ >   ('(' > expr > ')')
                    [ _val = construct_movable_(new_<ValueRef::UserStringLookup<std::string>>(deconstruct_movable_(_1, _pass))) ]
                )
                |   (
                    primary_expr [ _val = _1 ]
                )
            )
            ;

        operated_expr
            =
            (
                (
                    function_expr [ _a = _1 ]
                    >>  lit('+') [ _c = ValueRef::OpType::PLUS ]
                    >>  function_expr [ _b = construct_movable_(new_<ValueRef::Operation<std::string>>(_c, deconstruct_movable_(_a, _pass), deconstruct_movable_(_1, _pass))) ]
                    [ _val = _b ]
                )
                |   (
                    function_expr [ push_back(_d, _1) ]     // template string
                    >>+('%' >   function_expr [ push_back(_d, _1) ] )   // must have at least one sub-string
                    [ _val = construct_movable_(new_<ValueRef::Operation<std::string>>(ValueRef::OpType::SUBSTITUTION, deconstruct_movable_vector_(_d, _pass))) ]
                )
                |   (
                    function_expr [ _val = _1 ]
                )
            )
            ;

        expr
            =   operated_expr
            ;

        detail::initialize_nonnumeric_statistic_parser<std::string>(
            statistic, tok, label, condition_parser, statistic_sub_value_ref);

        primary_expr
            =   constant
            |   free_variable
            |   bound_variable
            |   statistic
            |   string_var_complex
            |   named_lookup_expr
            |   named_string_valueref
            ;

        bound_variable_name.name("string bound_variable name (e.g., Name)");
        constant.name(std::string{"quoted string or "}.append(ValueRef::Constant<std::string>::current_content));
        free_variable.name("free string variable");
        bound_variable.name("string bound_variable");
        statistic.name("string statistic");
        named_string_valueref.name("named string valueref");
        named_lookup_expr.name("string valueref lookup");
        expr.name("string expression");

#if DEBUG_VALUEREF_PARSERS
        debug(bound_variable_name);
        debug(constant);
        debug(free_variable);
        debug(bound_variable);
        debug(statistic);
        debug(named_string_valueref);
        debug(named_lookup_expr);
        debug(expr);
#endif
    }

    detail::name_token_rule bound_variable_name;
    detail::value_ref_rule<std::string> constant;
    detail::value_ref_rule<std::string> free_variable;
    detail::variable_rule<std::string> bound_variable;
    detail::value_ref_rule<std::string> statistic_sub_value_ref;
    detail::statistic_rule<std::string> statistic;
    detail::expression_rule<std::string> function_expr;
    detail::expression_rule<std::string> operated_expr;
    detail::value_ref_rule<std::string> expr;
    detail::value_ref_rule<std::string> primary_expr;
    detail::reference_token_rule variable_scope_rule;
    detail::container_token_rule container_type_rule;


    bool string_free_variable(std::string& text) {
        boost::spirit::qi::in_state_type in_state;

        const auto& tok = GetLexer();

        text_iterator first = text.begin();
        text_iterator last = text.end();
        token_iterator it = tok.begin(first, last);

        bool success = boost::spirit::qi::phrase_parse(
            it, tok.end(), tok.GalaxySeed_, in_state("WS")[tok.self]);

        return success;
    }
}
