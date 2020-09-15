/* ***** BEGIN LICENSE BLOCK *****
 * Distributed under the BSD license:
 *
 * Copyright (c) 2012, Ajax.org B.V.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Ajax.org B.V. nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AJAX.ORG B.V. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * ***** END LICENSE BLOCK ***** */

ace.define('ace/mode/proto', ['require', 'exports', 'module' , 'ace/lib/oop', 'ace/mode/text', 'ace/tokenizer', 'ace/mode/proto_highlight_rules'], function(require, exports, module) {


var oop = require("../lib/oop");
var TextMode = require("./text").Mode;
var Tokenizer = require("../tokenizer").Tokenizer;
var ProtoHighlightRules = require("./proto_highlight_rules").ProtoHighlightRules;

var Mode = function() {
    var highlighter = new ProtoHighlightRules();
    
    this.$tokenizer = new Tokenizer(highlighter.getRules());
};
oop.inherits(Mode, TextMode);

(function() {
       
    this.lineCommentStart = ";";
    
}).call(Mode.prototype);

exports.Mode = Mode;
});


ace.define('ace/mode/proto_highlight_rules', ['require', 'exports', 'module' , 'ace/lib/oop', 'ace/mode/text_highlight_rules'], function(require, exports, module) {


var oop = require("../lib/oop");
var TextHighlightRules = require("./text_highlight_rules").TextHighlightRules;

var ProtoHighlightRules = function() {
    var keywordControl = "case|let|let\*|letfed|rep|if|cond|case";
    var keywordOperator = "seq|all|and|or|mux|muxand|muxor|not|xor|loop";
    var constantLanguage = "null|quote|e|pi|inf";
    var supportFunctions = "select|apply|id|rep|dt|set-dt|fold-time|all-time|any-time|max-time|min-time|int-time|once|neg|mod|pow|exp|log|log10|logN|floor|ceil|max|min|denormalize|denormalizeN|is-zero|is-neg|is-pos|sqrt|abs|sin|cos|tan|asin|acos|atan2|sinh|cosh|tanh|asinh|acosh|atanh|rnd|rndint|vdot|vlen|normalize|polar-to-rect|rect-to-polar|rotate|tuple|len|elt|nul-tup|map|fold|slice|1st|2nd|3rd|find|position|assoc|defstruct|nbr|nbr-range|nbr-angle|nbr-lag|nbr-vec|is-self|infinitesimal|min-hood|min-hood+|max-hood|max-hood+|all-hood|all-hood+|any-hood|any-hood+|sum-hood|int-hood|fold-hood|fold-hood\*|fold-hood-plus|fold-hood-plus\*|mix|mov|speed|bearing|area|hood-radius|flex|mid|distance-to|broadcast|dilate|distance|disperse|dither|elect|flip|timer|tup|blue|red|green|sense";

    var keywordMapper = this.createKeywordMapper({
        "keyword.control": keywordControl,
        "keyword.operator": keywordOperator,
        "constant.language": constantLanguage,
        "support.function": supportFunctions
    }, "identifier", true);

    this.$rules = 
        {
    "start": [
        {
            token : "comment",
            regex : ";.*$"
        },
        {
            token: ["storage.type.function-type.proto", "text", "entity.name.function.proto"],
            regex: "(?:\\b(?:(def|fun))\\b)(\\s+)((?:\\w|\\-|\\!|\\?)*)"
        },
        {
            token: ["punctuation.definition.constant.character.proto", "constant.character.proto"],
            regex: "(#)((?:\\w|[\\\\+-=<>'\"&#])+)"
        },
        {
            token: ["punctuation.definition.variable.proto", "variable.other.global.proto", "punctuation.definition.variable.proto"],
            regex: "(\\*)(\\S*)(\\*)"
        },
        {
            token : "constant.numeric", // hex
            regex : "0[xX][0-9a-fA-F]+(?:L|l|UL|ul|u|U|F|f|ll|LL|ull|ULL)?\\b"
        }, 
        {
            token : "constant.numeric", // float
            regex : "[+-]?\\d+(?:(?:\\.\\d*)?(?:[eE][+-]?\\d+)?)?(?:L|l|UL|ul|u|U|F|f|ll|LL|ull|ULL)?\\b"
        },
        {
                token : keywordMapper,
                regex : "[a-zA-Z_$][\\-a-zA-Z0-9_$]*\\b"
        },
        {
            token : "string",
            regex : '"(?=.)',
            next  : "qqstring"
        }
    ],
    "qqstring": [
        {
            token: "constant.character.escape.proto",
            regex: "\\\\."
        },
        {
            token : "string",
            regex : '[^"\\\\]+'
        }, {
            token : "string",
            regex : "\\\\$",
            next  : "qqstring"
        }, {
            token : "string",
            regex : '"|$',
            next  : "start"
        }
    ]
}

};

oop.inherits(ProtoHighlightRules, TextHighlightRules);

exports.ProtoHighlightRules = ProtoHighlightRules;
});
