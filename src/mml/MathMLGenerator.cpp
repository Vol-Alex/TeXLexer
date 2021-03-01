#include "MathMLGenerator.h"
#include "src/Lexer.h"

#include <limits>
#include <memory>
#include <unordered_map>

namespace TXL
{
namespace
{
const auto MAX_INDEX = std::numeric_limits<std::size_t>::max();

class Builder
{
public:
    Builder() = default;
    virtual ~Builder() = default;

    virtual bool add(const Token& token) = 0;
    virtual std::string take() = 0;
};

const std::unordered_map<std::string, std::string>& getSymbolCmdMap()
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
        {"mu", "\xCE\xBС"},
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

        //Symbols
        {"pm", "\xC2\xB1"},
        {"approx", "\xE2\x89\x88"},
        {"propto", "\xE2\x88\x9D"},
        {"ne", "\xE2\x89\xA0"},
        {"neq", "\xE2\x89\xA0"},
        {"le", "\xE2\x89\xA4"},
        {"leq", "\xE2\x89\xA4"},
        {"ge", "\xE2\x89\xA5"},
        {"geq", "\xE2\x89\xA5"},
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
    RowBuilder(std::string&& nodeName = "mrow", const bool isInEnv = false)
        : _nodeName(std::move(nodeName))
        , _isInEnv(isInEnv)
    {
        _out.append("<").append(_nodeName).append(">");
        _lastTokenPos = _out.size();
    }

    bool add(const Token& token) override
    {
        if (_nestedBuilder)
        {
            if (_nestedBuilder->add(token))
            {
                return true;
            }
            _out.append(_nestedBuilder->take());
            _nestedBuilder.reset();
        }

        switch (token.type)
        {
            case COMMAND:
            {
                const auto& content = token.content;

                auto symbolCmdIt = getSymbolCmdMap().find(content);
                if (symbolCmdIt != getSymbolCmdMap().end())
                {
                    append("mi", symbolCmdIt->second);
                    return true;
                }

                auto it = getBuilderFactory().find(content);
                if (it != getBuilderFactory().end())
                {
                    _lastTokenPos = _out.size();
                    _nestedBuilder = (*it->second)();
                    return true;
                }
            }

            case TEXT:
                append("mi", token.content);
                return true;

            case DIGIT:
                append("mn", token.content);
                return true;

            case SIGN:
            {
                switch (token.content[0])
                {
                    case '^':
                    {
                        _nestedBuilder = makeSUP(_out.substr(_lastTokenPos));
                        _out.erase(_lastTokenPos);
                        return true;
                    }
                    case '_':
                    {
                        _nestedBuilder = makeSUB(_out.substr(_lastTokenPos));
                        _out.erase(_lastTokenPos);
                        return true;
                    }

                    case '&':
                    case '\\':
                    {
                        if (_isInEnv)
                        {
                            return false;
                        }
                    }

                    default:
                        break;
                }
                append("mo", token.content);
                return true;
            }

            case START_GROUP:
            case END_GROUP:
                return true;

            case BEGIN_ENV:
            {
                _lastTokenPos = _out.size();
                _nestedBuilder = makeEnvBuilder();
                return true;
            }

            case END_ENV:
            case END:
                break;
        }
        return false;
    }

    std::string take() override
    {
        _out.append("</").append(_nodeName).append(">");
        return std::move(_out);
    }

private:
    void append(const char* xmlNodeName, const std::string& content)
    {
        _lastTokenPos = _out.size();
        _out.append("<").append(xmlNodeName).append(">")
            .append(content.data(), content.size())
            .append("</").append(xmlNodeName).append(">");
    }

private:
    std::string _nodeName;
    std::string _out;
    std::unique_ptr<Builder> _nestedBuilder;
    std::size_t _lastTokenPos;
    bool _isInEnv;
};

class OptArgBuilder final : public Builder
{
public:
    bool add(const Token& token) override
    {
        if (_groupIndex == MAX_INDEX) return false;

        if (_groupIndex == 0 && token.content[0] != '[')
        {
            _groupIndex = MAX_INDEX;
            return false;
        }

        switch (token.type)
        {
            case START_GROUP:
                ++_groupIndex;
                break;

            case END_GROUP:
                --_groupIndex;
                if (_groupIndex == 0 && token.content[0] == ']') _groupIndex = MAX_INDEX;
                break;

            default:
                break;
        }

        return _rowBuilder.add(token);
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
    bool add(const Token& token) override
    {
        if (_groupIndex == MAX_INDEX)
        {
            return false;
        }

        switch (token.type)
        {
            case START_GROUP:
            {
                ++_groupIndex;
                if (_groupIndex == 1 && token.content[0] != '{') --_groupIndex;
                break;
            }

            case END_GROUP:
            {
                --_groupIndex;
                break;
            }

            default:
                break;
        }

        if (_groupIndex == 0) _groupIndex = MAX_INDEX;

        return _rowBuilder.add(token);
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
        bool add(const Token& token) override
        {
            return _arg1.add(token) || _arg2.add(token);
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

std::unique_ptr<Builder> makeSQRT()
{
    class SQRTBuilder final : public Builder
    {
        bool add(const Token& token) override
        {
            return _arg1.add(token) || _arg2.add(token);
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

std::unique_ptr<Builder> makeLeftRight()
{
    class LeftRightBuilder final : public Builder
    {
    public:
        bool add(const Token& token) override
        {
            if (!_ch.empty())
            {
                return false;
            }

            switch (token.type)
            {
                case SIGN:
                case START_GROUP:
                case END_GROUP:
                {
                    switch (token.content[0])
                    {
                        case '(':
                        case ')':
                        case '[':
                        case ']':
                            _ch = token.content;
                            return true;

                        default:
                            break;
                    }
                }

                default:
                    break;
            }
            return false;
        }

        std::string take() override
        {
            return R"(<mo fence="true" stretchy="true">)" + _ch + "</mo>";
        }

    private:
        std::string _ch;
    };

    return std::make_unique<LeftRightBuilder>();
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

    bool add(const Token& token) override
    {
        if (_arg.add(token))
        {
            return true;
        }

        if (_state != State::None)
        {
            return false;
        }

        switch(token.type)
        {
            case SIGN:
            {
                if (token.content[0] == '^' && _xmlNodeName == "msub")
                {
                    installState(State::SubSup);
                    return true;
                }
                else if (token.content[0] == '_' && _xmlNodeName == "msup")
                {
                    installState(State::SupSub);
                    return true;
                }
            }

            default:
                break;
        }
        return false;
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
        bool add(const Token& token) override
        {
            if (_rowBeginPos == MAX_INDEX)
            {
                return false;
            }

            if (_arg.add(token))
            {
                return true;
            }

            if (_tdBuilder.add(token))
            {
                return true;
            }

            switch (token.type)
            {
                case SIGN:
                {
                    switch (token.content[0])
                    {
                        case '&':
                        {
                            _out.append(_tdBuilder.take());
                            _tdBuilder = RowBuilder("mtd", true);
                            return true;
                        }

                        case '\\':
                        {
                            _out.insert(_rowBeginPos, "<mtr>");
                            _out.append(_tdBuilder.take()).append("</mtr>");
                            _rowBeginPos = _out.size();
                            _tdBuilder = RowBuilder("mtd", true);
                            return true;
                        }
                        default:
                            break;
                    }
                }

                case END_ENV:
                {
                    if (token.content == "matrix")
                    {
                        auto result = _tdBuilder.take();
                        if (result.size() > 11)
                        {
                            _out.insert(_rowBeginPos, "<mtr>");
                            _out.append(std::move(result)).append("</mtr>");
                        }
                        _rowBeginPos = MAX_INDEX;
                        return true;
                    }
                }

                default:
                    break;
            }
            return false;
        }

        std::string take() override
        {
            _out.append("</mtable>");
            return std::move(_out);
        }

    private:
        struct Arg final
        {
            bool add(const Token& token)
            {
                if (isProcessed)
                {
                    return false;
                }

                switch (token.type)
                {
                    case START_GROUP:
                    {
                        if (token.content[0] == '{')
                        {
                            data += token.content;
                            return true;
                        }
                    }

                    case END_GROUP:
                    {
                        if (token.content[0] == '}')
                        {
                            data += token.content;
                            isProcessed = true;
                            return true;
                        }
                    }

                    default:
                    {
                        if (!data.empty())
                        {
                            data += token.content;
                            return true;
                        }
                        break;
                    }
                }

                isProcessed = true;
                return false;
            }

            std::string data;
            bool isProcessed = false;
        };

    private:
        Arg _arg;
        std::string _out = std::string("<mtable>");
        std::size_t _rowBeginPos = _out.size();
        RowBuilder _tdBuilder = RowBuilder("mtd", true);
    };

    return std::make_unique<EnvBuilder>();
}

const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()>& getBuilderFactory()
{
    static const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()> map =
    {
    {"frac", makeFRAC},
    {"sqrt", makeSQRT},
    {"left", makeLeftRight},
    {"right", makeLeftRight},
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
    _out << R"(<?xml version="1.0" encoding="UTF-8"?>)" << std::endl;
    _out << R"(<math xmlns="http://www.w3.org/1998/Math/MathML">)" << std::endl;
    _out << R"(<mstyle displaystyle="true">)" << std::endl;

    RowBuilder builder;
    while(builder.add(lexer.next()));

    _out << builder.take();
    _out << R"(</mstyle>)" << std::endl;
    _out << R"(</math>)" << std::endl;
}
} // namespace TXL
