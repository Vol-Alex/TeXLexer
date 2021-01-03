#include "MathMLGenerator.h"

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

const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()>& getBuilderFactory();
std::unique_ptr<Builder> makeSUP();

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
                auto it = getBuilderFactory().find(std::string(token.content));
                if (it != getBuilderFactory().end())
                {
                    _nestedBuilder = (*it->second)();
                    return true;
                }
            }

            case TEXT:
            {
                _lastTokenPos = _out.size();
                _out.append(R"(<mi>)").append(token.content.data(), token.content.size()).append(R"(</mi>)");
                return true;
            }

            case START_GROUP:
            case END_GROUP:
            {
                return true;
            }

            case SIGN:
            {
                if (token.content[0] == '^')
                {
                    _out.insert(_lastTokenPos, R"(<msup><mrow>)");
                    _out.append(R"(</mrow>)");
                    _nestedBuilder = makeSUP();
                    return true;
                }
                _lastTokenPos = _out.size();
                _out.append(R"(<mo>)").append(token.content.data(), token.content.size()).append(R"(</mo>)");
                return true;
            }

            case END:
                break;
        }
        return false;
    }

    std::string take() override
    {
        _out.insert(0, R"(<mrow>)");
        _out.append(R"(</mrow>)");
        return std::move(_out);
    }

private:
    std::string _out;
    std::unique_ptr<Builder> _nestedBuilder;
    std::size_t _lastTokenPos = 0;
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
                if (_groupIndex == 0 && token.content[0] == '{') ++_groupIndex;
                else ++_groupIndex;
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

        if (_groupIndex == 0)
        {
            _groupIndex = MAX_INDEX;
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

std::unique_ptr<Builder> makeSUP()
{
    class SUPBuilder final : public Builder
    {
        bool add(const Token& token) override
        {
            return _arg.add(token);
        }

        std::string take() override
        {
            auto out = _arg.take();
            out.append(R"(</msup>)");
            return out;
        }

    private:
        ArgBuilder _arg;
    };
    return std::make_unique<SUPBuilder>();
}

const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()>& getBuilderFactory()
{
    static const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()> map =
    {
    {"sqrt", makeSQRT},
    {"^", makeSUP},
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
    _out << R"(<?xml version="1.0"?>)" << std::endl;
    _out << R"(<math xmlns="http://www.w3.org/1998/Math/MathML">)" << std::endl;
    _out << R"(<mstyle displaystyle="true">)" << std::endl;

    Lexer lexer(tex);
    RowBuilder builder;
    while(builder.add(lexer.next()));

    _out << builder.take();
    _out << R"(</mstyle>)" << std::endl;
    _out << R"(</math>)" << std::endl;
}
} // namespace TXL
