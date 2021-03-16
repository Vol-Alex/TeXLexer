#include "MathMLGenerator.h"
#include "src/Lexer.h"

#include <limits>
#include <memory>
#include <unordered_map>
#include <stack>

namespace TXL
{
namespace
{
const auto MAX_INDEX = std::numeric_limits<std::size_t>::max();

class TokenSequence final
{
public:
    TokenSequence(Lexer& lexer)
        : _lexer(lexer)
        , _t(lexer.next())
    {}

    const Token& top() const
    {
        return _t;
    }

    TokenSequence& next()
    {
        _t = _lexer.next();
        return *this;
    }

    bool empty() const
    {
        return _t.type == END;
    }

private:
    Lexer& _lexer;
    Token _t;
};

class Builder
{
public:
    Builder() = default;
    virtual ~Builder() = default;

    virtual void add(TokenSequence& sequence) = 0;
    virtual std::string take() = 0;
};

const std::unordered_map<std::string, std::string>& getCharCmdMap()
{
    static const std::unordered_map<std::string, std::string> map = {
        // Greek letters
        {"alpha", "\xCE\xB1"},
        {"beta", "\xCE\xB2"},
        {"Gamma", "\xCE\x93"},
        {"gamma", "\xCE\xB3"},
        {"Delta", "\xCE\x94"},
        {"delta", "\xCE\xB4"},
        {"epsilon", "\xCE\xB5"},
        {"zeta", "\xCE\xB6"},
        {"eta", "\xCE\xB7"},
        {"Theta", "\xCE\x98"},
        {"theta", "\xCE\xB8"},
        {"iota", "\xCE\xB9"},
        {"kappa", "\xCE\xBA"},
        {"Lambda", "\xCE\x9B"},
        {"lambda", "\xCE\xBB"},
        {"mu", "\xCE\xBC"},
        {"nu", "\xCE\xBD"},
        {"Xi", "\xCE\x9E"},
        {"xi", "\xCE\xBE"},
        {"Pi", "\xCE\xA0"},
        {"pi", "\xCF\x80"},
        {"rho", "\xCF\x81"},
        {"Sigma", "\xCE\xA3"},
        {"sigma", "\xCF\x83"},
        {"tau", "\xCF\x84"},
        {"Upsilon", "\xCE\xA5"},
        {"upsilon", "\xCF\x85"},
        {"Phi", "\xCE\xA6"},
        {"phi", "\xCF\x86"},
        {"chi", "\xCF\x87"},
        {"Psi", "\xCE\xA8"},
        {"psi", "\xCF\x88"},
        {"Omega", "\xCE\xA9"},
        {"omega", "\xCF\x89"},
        {"varsigma", "\xCF\x82"},
        {"vartheta", "\xCF\x91"},
        {"varphi", "\xCF\x95"},
        {"varpi", "\xCF\x96"},
        {"varrho", "\xCF\xB1"},
        {"varepsilon", "\xCF\xB5"},

        {"dots", "\xE2\x80\xA6"},
        {"ldots", "\xE2\x80\xA6"},
        {"vdots", "\xE2\x8B\xAE"},
        {"cdots", "\xE2\x8B\xAF"},
        {"ddots", "\xE2\x8B\xB1"},
        {"udots", "\xE2\x8B\xB0"},
    };
    return map;
}

const std::unordered_map<std::string, std::string>& getSymbolCmdMap()
{
    static const std::unordered_map<std::string, std::string> map = {
        {"approx", "\xE2\x89\x88"},
        {"cdot", "\xE2\x8B\x85"},
        {"ge", "\xE2\x89\xA5"},
        {"geq", "\xE2\x89\xA5"},
        {"infinity", "\xE2\x88\x9E"},
        {"infty", "\xE2\x88\x9E"},
        {"int","\xE2\x88\xAB"},
        {"integral","\xE2\x88\xAB"},
        {"le", "\xE2\x89\xA4"},
        {"leftarrow", "\xE2\x86\x90"},
        {"leq", "\xE2\x89\xA4"},
        {"ne", "\xE2\x89\xA0"},
        {"neq", "\xE2\x89\xA0"},
        {"pm", "\xC2\xB1"},
        {"propto", "\xE2\x88\x9D"},
        {"rightarrow", "\xE2\x86\x92"},
        {"to", "\xE2\x86\x92"},
    };
    return map;
}

const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()>& getBuilderFactory();

std::unique_ptr<Builder> makeEnvBuilder();
std::unique_ptr<Builder> makeSUP(std::string&& firstArg);
std::unique_ptr<Builder> makeSUB(std::string&& firstArg);

class RowBuilder final : public Builder
{
public:
    RowBuilder(std::string&& nodeName = "mrow")
        : _nodeName(std::move(nodeName))
    {
        _out.append("<").append(_nodeName).append(">");
        _lastTokenPos = _out.size();
    }

    void add(TokenSequence& sequence) override
    {
        const auto append = [&](const char* xmlNodeName, const std::string& content)
        {
            _lastTokenPos = _out.size();
            _out.append("<").append(xmlNodeName).append(">")
                .append(content.data(), content.size())
                .append("</").append(xmlNodeName).append(">");
            sequence.next();
        };

        const auto& token = sequence.top();
        switch (token.type)
        {
            case COMMAND:
            {
                const auto& content = token.content;

                auto symbolCmdIt = getSymbolCmdMap().find(content);
                if (symbolCmdIt != getSymbolCmdMap().end())
                {
                    append("mo", symbolCmdIt->second);
                    return;
                }

                auto chCmdIt = getCharCmdMap().find(content);
                if (chCmdIt != getCharCmdMap().end())
                {
                    append("mi", chCmdIt->second);
                    return;
                }


                if (content == "left")
                {
                    _fences.push({_out.size(), sequence.next().top().content});
                    sequence.next();
                    return;
                }

                if (content == "right" && !_fences.empty())
                {
                    const auto& top = _fences.top();
                    _lastTokenPos = top.first;
                    _out.insert(top.first, "<mfenced open='" + top.second +
                                "' close='" + sequence.next().top().content + "'><mrow>");
                    _out.append("</mrow></mfenced>");
                    _fences.pop();
                    sequence.next();
                    return;
                }

                auto builderIt = getBuilderFactory().find(content);
                if (builderIt != getBuilderFactory().end())
                {
                    _lastTokenPos = _out.size();
                    auto nestedBuilder = (*builderIt->second)();
                    nestedBuilder->add(sequence.next());
                    _out.append(nestedBuilder->take());
                    return;
                }
            }

            case TEXT:
                append("mi", token.content);
                return;

            case DIGIT:
                append("mn", token.content);
                return;

            case SIGN:
            {
                switch (token.content[0])
                {
                    case '^':
                    {
                        auto nestedBuilder = makeSUP(_out.substr(_lastTokenPos));
                        _out.erase(_lastTokenPos);
                        nestedBuilder->add(sequence.next());
                        _out.append(nestedBuilder->take());
                        return;
                    }
                    case '_':
                    {
                        auto nestedBuilder = makeSUB(_out.substr(_lastTokenPos));
                        _out.erase(_lastTokenPos);
                        nestedBuilder->add(sequence.next());
                        _out.append(nestedBuilder->take());
                        return;
                    }
                    case '<':
                        append("mo", "&lt;");
                        return;

                    case '>':
                        append("mo", "&gt;");
                        return;

                    default:
                        break;
                }
                append("mo", token.content);
                return;
            }

            case BEGIN_ENV:
            {
                _lastTokenPos = _out.size();
                auto nestedBuilder = makeEnvBuilder();
                nestedBuilder->add(sequence.next());
                _out.append(nestedBuilder->take());
                return;
            }

            case START_GROUP:
            case END_GROUP:
            case END_ENV:
                sequence.next();
                break;

            case END:
                break;
        }
    }

    std::string take() override
    {
        _out.append("</").append(_nodeName).append(">");
        return std::move(_out);
    }

private:
    std::string _nodeName;
    std::string _out;
    std::size_t _lastTokenPos;
    std::stack<std::pair<std::size_t, std::string>> _fences;
};

class OptArgBuilder final : public Builder
{
public:
    void add(TokenSequence& sequence) override
    {
        if (sequence.top().content[0] != '[')
        {
            return;
        }

        for(bool finalize = false; !finalize && !sequence.empty();)
        {
            const auto& token = sequence.top();
            switch (token.type)
            {
                case START_GROUP:
                    ++_groupIndex;
                    break;

                case END_GROUP:
                    --_groupIndex;
                    if (_groupIndex == 0 && token.content[0] == ']') finalize = true;
                    break;

                default:
                    break;
            }
            _rowBuilder.add(sequence);
        }
    }

    std::string take() override
    {
        return _rowBuilder.take();
    }

private:
    std::size_t _groupIndex = 0;
    RowBuilder _rowBuilder;
};

class ArgBuilder final : public Builder
{
public:
    void add(TokenSequence& sequence) override
    {
        if (sequence.top().content[0] != '{')
        {
            _rowBuilder.add(sequence);
            return;
        }

        for(bool finalize = false; !finalize && !sequence.empty();)
        {
            const auto& token = sequence.top();
            switch (token.type)
            {
                case START_GROUP:
                    ++_groupIndex;
                    break;

                case END_GROUP:
                    --_groupIndex;
                    if (_groupIndex == 0 && token.content[0] == '}') finalize = true;
                    break;

                default:
                    break;
            }
            _rowBuilder.add(sequence);
        }
    }

    std::string take() override
    {
        return _rowBuilder.take();
    }

private:
    std::size_t _groupIndex = 0;
    RowBuilder _rowBuilder;
};

std::unique_ptr<Builder> makeFRAC()
{
    class FRACBuilder final : public Builder
    {
        void add(TokenSequence& sequence) override
        {
            _arg1.add(sequence);
            _arg2.add(sequence);
        }

        std::string take() override
        {
            std::string out(R"(<mfrac>)");
            out.append(_arg1.take());
            out.append(_arg2.take());
            out.append(R"(</mfrac>)");
            return out;
        }

    private:
        ArgBuilder _arg1;
        ArgBuilder _arg2;
    };
    return std::make_unique<FRACBuilder>();
}

std::unique_ptr<Builder> makeGENFRAC()
{
    class GENFRACBuilder final : public Builder
    {
        void add(TokenSequence& sequence) override
        {
            _left.add(sequence);
            _right.add(sequence);
            _barThickness.add(sequence);
            _style.add(sequence);
            _numerator.add(sequence);
            _denominator.add(sequence);
        }

        std::string take() override
        {
            std::string out;
            out.append("<mfenced open='").append(_left.data).append("' close='").append(_right.data).append("'><mrow>")
               .append("<mfrac linethickness='").append(_barThickness.data).append("'>")
               .append(_numerator.take())
               .append(_denominator.take())
               .append("</mfrac></mrow></mfenced>");
            return out;
        }

    private:
        struct Arg final
        {
            void add(TokenSequence& sequence)
            {
                const auto& token = sequence.top();
                if (token.content[0] != '{')
                {
                    data += token.content;
                    sequence.next();
                    return;
                }

                for(bool finalize = false; !finalize && !sequence.empty(); sequence.next())
                {
                    const auto& token = sequence.top();
                    switch (token.type)
                    {
                        case START_GROUP:
                            ++_groupIndex;
                            break;

                        case END_GROUP:
                            --_groupIndex;
                            if (_groupIndex == 0 && token.content[0] == '}') finalize = true;
                            break;

                        default:
                            data += token.content;
                            break;
                    }
                }
            }

        public:
            std::string data;

        private:
            std::size_t _groupIndex = 0;
        };

    private:
        Arg _left;
        Arg _right;
        Arg _barThickness;
        Arg _style;
        ArgBuilder _numerator;
        ArgBuilder _denominator;
    };
    return std::make_unique<GENFRACBuilder>();
}

std::unique_ptr<Builder> makeSQRT()
{
    class SQRTBuilder final : public Builder
    {
        void add(TokenSequence& sequence) override
        {
            _arg1.add(sequence);
            _arg2.add(sequence);
        }

        std::string take() override
        {
            std::string out(R"(<mroot>)");
            out.append(_arg2.take());
            out.append(_arg1.take());
            out.append(R"(</mroot>)");
            return out;
        }

    private:
        OptArgBuilder _arg1;
        ArgBuilder _arg2;
    };
    return std::make_unique<SQRTBuilder>();
}

class SUPSUBBuilder final : public Builder
{
    enum class State
    {
        None,
        SupSub,
        SubSup,
    };
public:
    SUPSUBBuilder(std::string&& xmlNodeName,
                  std::string&& firstArg)
        : _xmlNodeName(std::move(xmlNodeName))
        , _firstArg(std::move(firstArg))
    {}

    void add(TokenSequence& sequence) override
    {
        _arg.add(sequence);

        const auto& token = sequence.top();
        switch(token.type)
        {
            case SIGN:
            {
                if (token.content[0] == '^' && _xmlNodeName == "msub")
                {
                    installState(State::SubSup);
                    _arg.add(sequence.next());
                }
                else if (token.content[0] == '_' && _xmlNodeName == "msup")
                {
                    installState(State::SupSub);
                    _arg.add(sequence.next());
                }
                break;
            }

            default:
                break;
        }
    }

    std::string take() override
    {
        _firstArg.insert(0, "<" + _xmlNodeName + "><mrow>");
        _firstArg.append("</mrow>");

        if (_state == State::SubSup) _firstArg += _additionalArg;

        _firstArg += _arg.take();

        if (_state == State::SupSub) _firstArg += _additionalArg;

        _firstArg.append("</").append(_xmlNodeName).append(">");
        return std::move(_firstArg);
    }

private:
    void installState(const State state)
    {
        _additionalArg = _arg.take();
        _arg = ArgBuilder();
        _state = state;
        _xmlNodeName = "msubsup";
    }

private:
    std::string _xmlNodeName;
    State _state = State::None;

    std::string _firstArg;
    std::string _additionalArg;
    ArgBuilder _arg;
};

std::unique_ptr<Builder> makeSUP(std::string&& firstArg)
{
    return std::make_unique<SUPSUBBuilder>("msup", std::move(firstArg));
}

std::unique_ptr<Builder> makeSUB(std::string&& firstArg)
{
    return std::make_unique<SUPSUBBuilder>("msub",  std::move(firstArg));
}

std::unique_ptr<Builder> makeEnvBuilder()
{
    class EnvBuilder final : public Builder
    {
    public:
        void add(TokenSequence& sequence) override
        {
            _arg.add(sequence);

            for(; !sequence.empty();)
            {
                const auto& token = sequence.top();
                switch (token.type)
                {
                    case SIGN:
                    {
                        switch (token.content[0])
                        {
                            case '&':
                            {
                                _out.append(_tdBuilder.take());
                                _tdBuilder = RowBuilder("mtd");
                                sequence.next();
                                break;
                            }

                            case '\\':
                            {
                                _out.insert(_rowBeginPos, "<mtr>");
                                _out.append(_tdBuilder.take()).append("</mtr>");
                                _rowBeginPos = _out.size();
                                _tdBuilder = RowBuilder("mtd");
                                sequence.next();
                                break;
                            }
                            default:
                                _tdBuilder.add(sequence);
                                break;
                        }
                        break;
                    }

                    case END_ENV:
                    {
                        auto result = _tdBuilder.take();
                        if (result.size() > 11)
                        {
                            _out.insert(_rowBeginPos, "<mtr>");
                            _out.append(std::move(result)).append("</mtr>");
                        }
                        sequence.next();
                        _rowBeginPos = MAX_INDEX;
                        return;
                    }

                    default:
                        _tdBuilder.add(sequence);
                        break;
                }
            }
        }

        std::string take() override
        {
            _out.append("</mtable>");
            return std::move(_out);
        }

    private:
        struct Arg final
        {
            void add(TokenSequence& sequence)
            {
                if (sequence.top().content[0] != '{')
                {
                    return;
                }

                for(bool finalize = false; !finalize && !sequence.empty(); sequence.next())
                {
                    const auto& token = sequence.top();
                    switch (token.type)
                    {
                        case START_GROUP:
                            ++_groupIndex;
                            break;

                        case END_GROUP:
                            --_groupIndex;
                            if (_groupIndex == 0 && token.content[0] == '}') finalize = true;
                            break;

                        default:
                            break;
                    }
                    data += token.content;
                }
            }

        public:
            std::string data;

        private:
            std::size_t _groupIndex = 0;
        };

    private:
        Arg _arg;
        std::string _out = std::string("<mtable>");
        std::size_t _rowBeginPos = _out.size();
        RowBuilder _tdBuilder = RowBuilder("mtd");
    };

    return std::make_unique<EnvBuilder>();
}

class SumLikeBuilder final : public Builder
{
public:
    SumLikeBuilder(std::string&& cmdNode)
        : _cmdNode(std::move(cmdNode))
    {
    }

    void add(TokenSequence& sequence) override
    {
        for (bool finalize = false; !finalize && !sequence.empty();)
        {
            const auto& token = sequence.top();
            switch (token.type)
            {
                case SIGN:
                {
                    switch (token.content[0])
                    {
                        case '^':
                        {
                            if (_hasSup)
                            {
                                finalize = true;
                                break;
                            }
                            _hasSup = true;
                            _sup.add(sequence.next());
                            break;
                        }
                        case '_':
                        {
                            if (_hasSub)
                            {
                                finalize = true;
                                break;
                            }
                            _hasSub = true;
                            _sub.add(sequence.next());
                            break;
                        }

                        default:
                            finalize = true;
                            break;
                    }
                    break;
                }

                default:
                    finalize = true;
                    break;
            }
        }
    }

    std::string take() override
    {
        if(_hasSub && _hasSup)
        {
            std::string out;
            out.append("<munderover>")
               .append("<mrow>")
               .append(_cmdNode)
               .append("</mrow>")
               .append(_sub.take())
               .append(_sup.take())
               .append("</munderover>");
            return out;
        }
        if(_hasSub)
        {
            std::string out;
            out.append("<munder>")
               .append("<mrow>")
               .append(_cmdNode)
               .append("</mrow>")
               .append(_sub.take())
               .append("</munder>");
            return out;
        }
        if(_hasSup)
        {
            std::string out;
            out.append("<mover>")
               .append("<mrow>")
               .append(_cmdNode)
               .append("</mrow>")
               .append(_sup.take())
               .append("</mover>");
            return out;
        }
        return _cmdNode;
    }

private:
    std::string _cmdNode;
    ArgBuilder _sub;
    bool _hasSub = false;
    ArgBuilder _sup;
    bool _hasSup = false;
};

std::unique_ptr<Builder> makeSUM()
{
    return std::make_unique<SumLikeBuilder>("<mo>\xE2\x88\x91</mo>");
}

std::unique_ptr<Builder> makePROD()
{
    return std::make_unique<SumLikeBuilder>("<mo>\xE2\x88\x8F</mo>");
}

std::unique_ptr<Builder> makeLIM()
{
    return std::make_unique<SumLikeBuilder>("<mi mathvariant=\"normal\">lim</mi>");
}

class ReverseTwoArgBuilder final : public Builder
{
public:
    ReverseTwoArgBuilder(std::string&& nodeName)
        : _nodeName(std::move(nodeName))
    {
    }

    void add(TokenSequence& sequence) override
    {
        _arg1.add(sequence);
        _arg2.add(sequence);
    }

    std::string take() override
    {
        std::string out;
        out.append("<").append(_nodeName).append(">")
           .append(_arg2.take())
           .append(_arg1.take())
           .append("</").append(_nodeName).append(">");
        return out;
    }

private:
    std::string _nodeName;
    ArgBuilder _arg1;
    ArgBuilder _arg2;
};

std::unique_ptr<Builder> makeOVERSET()
{
    return std::make_unique<ReverseTwoArgBuilder>("mover");
}

std::unique_ptr<Builder> makeUNDERSET()
{
    return std::make_unique<ReverseTwoArgBuilder>("munder");
}

std::unique_ptr<Builder> makeMATHRM()
{
    class MATHRMBuilder final : public Builder
    {
        void add(TokenSequence& sequence) override
        {
            _arg.add(sequence);
        }

        std::string take() override
        {
            std::string out(R"(<mstyle mathvariant="normal">)");
            out.append(_arg.take())
               .append(R"(</mstyle>)");
            return out;
        }

    private:
        ArgBuilder _arg;
    };
    return std::make_unique<MATHRMBuilder>();
}

std::unique_ptr<Builder> makeOVERLINE()
{
    class OVERLINEBuilder final : public Builder
    {
        void add(TokenSequence& sequence) override
        {
            _arg.add(sequence);
        }

        std::string take() override
        {
            std::string out(R"(<mover>)");
            out.append(_arg.take())
               .append("<mo>\xC2\xAF</mo></mover>");
            return out;
        }

    private:
        ArgBuilder _arg;
    };
    return std::make_unique<OVERLINEBuilder>();
}

std::unique_ptr<Builder> makeHSPACE()
{
    class HSPACEBuilder final : public Builder
    {
        void add(TokenSequence& sequence) override
        {
            _arg.add(sequence);
        }

        std::string take() override
        {
            return "<mo>\xE2\x80\x89</mo>";
        }

    private:
        ArgBuilder _arg;
    };
    return std::make_unique<HSPACEBuilder>();
}

const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()>& getBuilderFactory()
{
    static const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()> map =
    {
        {"closure", makeOVERLINE},
        {"frac", makeFRAC},
        {"genfrac", makeGENFRAC},
        {"hspace", makeHSPACE},
        {"lim", makeLIM},
        {"mathrm", makeMATHRM},
        {"overline", makeOVERLINE},
        {"overset", makeOVERSET},
        {"prod", makePROD},
        {"product", makePROD},
        {"sqrt", makeSQRT},
        {"stackrel", makeOVERSET},
        {"sum", makeSUM},
        {"underset", makeUNDERSET},
        {"widebar", makeOVERLINE},
    };
    return map;
}

} // namespace

MathMLGenerator::MathMLGenerator(std::ostream& out)
    : _out(out)
{
}

MathMLGenerator::~MathMLGenerator() = default;

void MathMLGenerator::generate(const std::string& tex)
{
    Lexer lexer(tex);
    generate(lexer);
}

void MathMLGenerator::generateFromIN()
{
    Lexer lexer;
    generate(lexer);
}

void MathMLGenerator::generate(Lexer& lexer)
{
    _out << R"(<?xml version="1.0" encoding="UTF-8"?>)" << std::endl
         << R"(<math xmlns="http://www.w3.org/1998/Math/MathML">)" << std::endl
         << R"(<mstyle displaystyle="true">)" << std::endl;

    TokenSequence sequence{lexer};
    RowBuilder builder;
    while(!sequence.empty()) builder.add(sequence);

    _out << builder.take()
         << R"(</mstyle>)" << std::endl
         << R"(</math>)" << std::endl;
}
} // namespace TXL
