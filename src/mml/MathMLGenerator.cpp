#include "MathMLGenerator.h"
#include "src/Lexer.h"

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
        {"Pi", "\xCE\xA0"},
        {"pi", "\xCF\x80"}
    };
    return map;
}

const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()>& getBuilderFactory();
std::unique_ptr<Builder> makeSUP();
std::unique_ptr<Builder> makeSUB();

class RowBuilder final : public Builder
{
public:
    bool add(const Token& token) override
    {
        if (_nestedBuilder)
        {
            if (_nestedBuilder->add(token))
            {
                return true;
            }
            _lastTokenPos = _out.size();
            _out.append(_nestedBuilder->take());
            _nestedBuilder.reset();
        }

        switch (token.type)
        {
            case COMMAND:
            {
                const auto content = std::string(token.content);

                auto symbolCmdIt = getSymbolCmdMap().find(content);
                if (symbolCmdIt != getSymbolCmdMap().end())
                {
                    append("mi", symbolCmdIt->second);
                    return true;
                }

                auto it = getBuilderFactory().find(content);
                if (it != getBuilderFactory().end())
                {
                    _nestedBuilder = (*it->second)();
                    return true;
                }
            }

            case DIGIT:
                append("mn", token.content);
                return true;

            case TEXT:
                append("mi", token.content);
                return true;

            case SIGN:
            {
                if (token.content[0] == '^')
                {
                    _out.insert(_lastTokenPos, "<msup><mrow>");
                    _out.append("</mrow>");
                    _nestedBuilder = makeSUP();
                    return true;
                }
                else if (token.content[0] == '_')
                {
                    _out.insert(_lastTokenPos, "<msub><mrow>");
                    _out.append("</mrow>");
                    _nestedBuilder = makeSUB();
                    return true;
                }
                append("mo", token.content);
                return true;
            }

            case START_GROUP:
            case END_GROUP:
                return true;

            case END:
                break;
        }
        return false;
    }

    std::string take() override
    {
        _out.append(R"(</mrow>)");
        return std::move(_out);
    }

private:
    void append(const char* xmlNodeName, const std::string_view content)
    {
        _lastTokenPos = _out.size();
        _out.append("<").append(xmlNodeName).append(">")
            .append(content.data(), content.size())
            .append("</").append(xmlNodeName).append(">");
    }

private:
    std::string _out = std::string("<mrow>");
    std::unique_ptr<Builder> _nestedBuilder;
    std::size_t _lastTokenPos = 6;
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

                case COMMAND:
                case DIGIT:
                case TEXT:
                case END:
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
public:
    SUPSUBBuilder(std::string&& xmlNodeName)
        : _xmlNodeName(std::move(xmlNodeName))
    {}

    bool add(const Token& token) override
    {
        return _arg.add(token);
    }

    std::string take() override
    {
        auto out = _arg.take();
        out.append("</").append(_xmlNodeName).append(">");
        return out;
    }

private:
    std::string _xmlNodeName;
    ArgBuilder _arg;
};

std::unique_ptr<Builder> makeSUP()
{
    return std::make_unique<SUPSUBBuilder>("msup");
}

std::unique_ptr<Builder> makeSUB()
{
    return std::make_unique<SUPSUBBuilder>("msub");
}

const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()>& getBuilderFactory()
{
    static const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()> map =
    {
    {"frac", makeFRAC},
    {"sqrt", makeSQRT},
    {"left", makeLeftRight},
    {"right", makeLeftRight},
    {"^", makeSUP},
    {"_", makeSUB},
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
