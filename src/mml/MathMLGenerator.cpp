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

    std::string popChar()
    {
        if (_t.type == TEXT)
        {
            auto& content = _t.content;
            auto result = content.substr(0, getCharLength(content[0]));
            content.erase(0, result.size());
            if (content.empty())
            {
                next();
            }
            return result;
        }
        return std::string();
    }

    bool empty() const
    {
        return _t.type == END;
    }

private:
    uint8_t getCharLength(const char firstByte)
    {
        uint8_t lead = static_cast<uint8_t>(firstByte);
        if (lead < 0x80)
            return 1;
        else if ((lead >> 5) == 0x6)
            return 2;
        else if ((lead >> 4) == 0xe)
            return 3;
        else if ((lead >> 3) == 0x1e)
            return 4;
        else
            return 0;
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
        {"varkappa", "\xCF\xB0"},
        {"varrho", "\xCF\xB1"},
        {"varepsilon", "\xCF\xB5"},

        {"dots", "\xE2\x80\xA6"},
        {"ldots", "\xE2\x80\xA6"},
        {"dotso", "\xE2\x80\xA6"},
        {"dotsc", "\xE2\x80\xA6"},
        {"vdots", "\xE2\x8B\xAE"},
        {"cdots", "\xE2\x8B\xAF"},
        {"dotsb", "\xE2\x8B\xAF"},
        {"ddots", "\xE2\x8B\xB1"},
        {"udots", "\xE2\x8B\xB0"},
        {"hbar", "\xE2\x84\x8F"},
    };
    return map;
}

const std::unordered_map<std::string, std::string>& getSymbolCmdMap()
{
    static const std::unordered_map<std::string, std::string> map = {
        {"Del", "\xE2\x88\x87"},
        {"Im", "\xE2\x84\x91"},
        {"Re", "\xE2\x84\x9C"},
        {"amalg", "\xE2\xA8\xBF"},
        {"angle", "\xE2\x88\xA0"},
        {"approx", "\xE2\x89\x88"},
        {"ast", "\xE2\x88\x97"},
        {"bigcap", "\xE2\x8B\x82"},
        {"bigcup", "\xE2\x8B\x83"},
        {"bigvee", "\xE2\x8B\x81"},
        {"bigwedge", "\xE2\x8B\x80"},
        {"bullet", "\xE2\x80\xA2"},
        {"cap", "\xE2\x88\xA9"},
        {"cdot", "\xE2\x8B\x85"},
        {"circ", "\xE2\x88\x98"},
        {"cong", "\xE2\x89\x85"},
        {"conint", "\xE2\x88\xAE"},
        {"contourintegral", "\xE2\x88\xAE"},
        {"coprod", "\xE2\x88\x90"},
        {"coproduct", "\xE2\x88\x90"},
        {"cup", "\xE2\x88\xAA"},
        {"div", "\xC3\xB7"},
        {"doubleintegral", "\xE2\x88\xAC"},
        {"downarrow", "\xE2\x86\x93"},
        {"equiv", "\xE2\x89\xA1"},
        {"exists", "\xE2\x88\x83"},
        {"forall", "\xE2\x88\x80"},
        {"ge", "\xE2\x89\xA5"},
        {"geq", "\xE2\x89\xA5"},
        {"gt", "&gt;"},
        {"hslash", "\xE2\x84\x8F"},
        {"in", "\xE2\x88\x8A"},
        {"infinity", "\xE2\x88\x9E"},
        {"infty", "\xE2\x88\x9E"},
        {"le", "\xE2\x89\xA4"},
        {"leftarrow", "\xE2\x86\x90"},
        {"leq", "\xE2\x89\xA4"},
        {"lt", "&lt;"},
        {"measuredangle", "\xE2\x88\xA1"},
        {"nabla", "\xE2\x88\x87"},
        {"ne", "\xE2\x89\xA0"},
        {"neg", "\xC2\xAC"},
        {"neq", "\xE2\x89\xA0"},
        {"nexists", "\xE2\x88\x84"},
        {"ngeq", "\xE2\x89\xB1"},
        {"nleq","\xE2\x89\xB0"},
        {"nless", "\xE2\x89\xAE"},
        {"not", "\x2F"},
        {"notin", "\xE2\x88\x89"},
        {"nparallel", "\xE2\x88\xA6"},
        {"nsubseteq", "\xE2\x8A\x88"},
        {"nsupseteq", "\xE2\x8A\x89"},
        {"odot", "\xE2\x8A\x99"},
        {"ominus", "\xE2\x8A\x96"},
        {"oplus", "\xE2\x8A\x95"},
        {"oslah", "\xE2\x8A\x98"},
        {"otimes", "\xE2\x8A\x97"},
        {"parallel", "\xE2\x88\xA5"},
        {"partial", "\xE2\x88\x82"},
        {"perp", "\xE2\x8A\xA5"},
        {"pm", "\xC2\xB1"},
        {"prime", "\xE2\x80\xB2"},
        {"propto", "\xE2\x88\x9D"},
        {"quadrupleintegral", "\xE2\xA8\x8C"},
        {"rightarrow", "\xE2\x86\x92"},
        {"setminus", "\xE2\x88\x96"},
        {"sim", "\xE2\x88\xBC"},
        {"simeq", "\xE2\x89\x83"},
        {"subset", "\xE2\x8A\x82"},
        {"subseteq","\xE2\x8A\x86"},
        {"supset", "\xE2\x8A\x83"},
        {"supseteq", "\xE2\x8A\x87"},
        {"times", "\xC3\x97"},
        {"to", "\xE2\x86\x92"},
        {"triangle", "\xE2\x96\xB3"},
        {"triangledown", "\xE2\x96\xBF"},
        {"tripleintegral", "\xE2\x88\xAD"},
        {"uparrow", "\xE2\x86\x91"},
        {"vee", "\xE2\x88\xA8"},
        {"wedge", "\xE2\x88\xA7"},
        {"wp", "\xE2\x84\x98"},
    };
    return map;
}

const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()>& getBuilderFactory();

enum class SubSupType
{
    Limits,
    NoLimits
};

std::unique_ptr<Builder> makeEnvBuilder(const std::string& name);
std::unique_ptr<Builder> makeSubSup(std::string&& firstArg, SubSupType type);

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
                    _out.insert(top.first, "<mfenced open='" + (top.second == "." ? "" : top.second) +
                                "' close='" + (sequence.next().top().content == "." ? "" : sequence.top().content) + "'><mrow>");
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
                    case '_':
                    {
                        auto nestedBuilder = makeSubSup(_out.substr(_lastTokenPos), SubSupType::NoLimits);
                        _out.erase(_lastTokenPos);
                        nestedBuilder->add(sequence);
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
                auto nestedBuilder = makeEnvBuilder(token.content);
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

    void addCharOrToken(TokenSequence& sequence)
    {
        auto charSequence = sequence.popChar();
        if (!charSequence.empty())
        {
            _out.append("<mi>")
                .append(charSequence.data(), charSequence.size())
                .append("</mi>");
            return;
        }
        add(sequence);
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
            _rowBuilder.addCharOrToken(sequence);
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

class TextArgBuilder final : public Builder
{
public:
    explicit TextArgBuilder(bool preserveWhitespace = false)
        : _preserveWhitespace(preserveWhitespace)
    {
    }

    void add(TokenSequence& sequence) override
    {
        if (sequence.top().content[0] != '{')
        {
            _out = sequence.top().content;
            sequence.next();
            return;
        }

        std::size_t groupIndex = 1;
        sequence.next();
        for(bool finalize = false; !finalize && !sequence.empty(); sequence.next())
        {
            const auto& token = sequence.top();
            switch (token.type)
            {
                case START_GROUP:
                    ++groupIndex;
                    break;

                case END_GROUP:
                    --groupIndex;
                    if (groupIndex == 0 && token.content[0] == '}') finalize = true;
                    break;

                default:
                    break;
            }

            if (!finalize)
            {
                if (_preserveWhitespace && !_out.empty()) _out.append(" ");
                _out.append(token.content);
            }
        }
    }

    std::string takeContent()
    {
        return std::move(_out);
    }

    std::string take() override
    {
        return "<mtext>" + _out + "</mtext>";
    }

private:
    std::string _out;
    bool _preserveWhitespace;
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
            out.append("<mfenced open='").append(_left.takeContent()).append("' close='").append(_right.takeContent()).append("'><mrow>")
               .append("<mfrac linethickness='").append(_barThickness.takeContent()).append("'>")
               .append(_numerator.take())
               .append(_denominator.take())
               .append("</mfrac></mrow></mfenced>");
            return out;
        }

    private:

    private:
        TextArgBuilder _left;
        TextArgBuilder _right;
        TextArgBuilder _barThickness;
        TextArgBuilder _style;
        ArgBuilder _numerator;
        ArgBuilder _denominator;
    };
    return std::make_unique<GENFRACBuilder>();
}

std::unique_ptr<Builder> makeBINOM()
{
    class BINOMBuilder final : public Builder
    {
        void add(TokenSequence& sequence) override
        {
            _numerator.add(sequence);
            _denominator.add(sequence);
        }

        std::string take() override
        {
            std::string out;
            out.append("<mfenced open='(' close=')'><mrow><mfrac linethickness='0pt'>")
               .append(_numerator.take())
               .append(_denominator.take())
               .append("</mfrac></mrow></mfenced>");
            return out;
        }
    private:
        ArgBuilder _numerator;
        ArgBuilder _denominator;
    };
    return std::make_unique<BINOMBuilder>();
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

class SubSupBuilder final : public Builder
{
public:
    SubSupBuilder(std::string&& cmdNode, SubSupType type)
        : _cmdNode(std::move(cmdNode))
        , _type(type)
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

                case COMMAND:
                {
                    const auto& content = token.content;

                    if (content == "limits")
                    {
                        _type = SubSupType::Limits;
                        sequence.next();
                        break;
                    }

                    if (content == "nolimits")
                    {
                        _type = SubSupType::NoLimits;
                        sequence.next();
                        break;
                    }
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
            out.append(_type == SubSupType::Limits ? "<munderover>" : "<msubsup>")
                    .append("<mrow>")
                    .append(_cmdNode)
                    .append("</mrow>")
                    .append(_sub.take())
                    .append(_sup.take())
                    .append(_type == SubSupType::Limits ? "</munderover>" : "</msubsup>");
            return out;
        }
        if(_hasSub)
        {
            std::string out;
            out.append(_type == SubSupType::Limits ? "<munder>" : "<msub>")
                    .append("<mrow>")
                    .append(_cmdNode)
                    .append("</mrow>")
                    .append(_sub.take())
                    .append(_type == SubSupType::Limits ? "</munder>" : "</msub>");
            return out;
        }
        if(_hasSup)
        {
            std::string out;
            out.append(_type == SubSupType::Limits ? "<mover>" : "<msup>")
                    .append("<mrow>")
                    .append(_cmdNode)
                    .append("</mrow>")
                    .append(_sup.take())
                    .append(_type == SubSupType::Limits ? "</mover>" : "</msup>");
            return out;
        }
        return _cmdNode;
    }

private:
    SubSupType _type;
    std::string _cmdNode;
    ArgBuilder _sub;
    bool _hasSub = false;
    ArgBuilder _sup;
    bool _hasSup = false;
};

std::unique_ptr<Builder> makeSubSup(std::string&& firstArg, SubSupType type)
{
    return std::make_unique<SubSupBuilder>(std::move(firstArg), type);
}

class TableBuilder final : public Builder
{
public:
    void add(TokenSequence& sequence) override
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

            default:
                _tdBuilder.add(sequence);
                break;
        }
    }

    std::string take() override
    {
        auto result = _tdBuilder.take();
        if (result.size() > 11)
        {
            _out.insert(_rowBeginPos, "<mtr>");
            _out.append(std::move(result)).append("</mtr>");
        }
        _rowBeginPos = MAX_INDEX;
        _out.append("</mtable>");

        return std::move(_out);
    }

private:
    std::string _out = std::string("<mtable>");
    std::size_t _rowBeginPos = _out.size();
    RowBuilder _tdBuilder = RowBuilder("mtd");
};

class ArgTableBuilder final : public Builder
{
public:
    void add(TokenSequence& sequence) override
    {
        if (sequence.top().content[0] != '{')
        {
            _tableBuilder.add(sequence);
            return;
        }

        auto groupIndex = 0;
        for(bool finalize = false; !finalize && !sequence.empty();)
        {
            const auto& token = sequence.top();
            switch (token.type)
            {
                case START_GROUP:
                    ++groupIndex;
                    break;

                case END_GROUP:
                    --groupIndex;
                    if (groupIndex == 0 && token.content[0] == '}') finalize = true;
                    break;

                default:
                    break;
            }
            _tableBuilder.add(sequence);
        }
    }

    std::string take() override
    {
        return _tableBuilder.take();
    }

private:
    TableBuilder _tableBuilder;
};

std::unique_ptr<Builder> makeEnvBuilder(const std::string& name)
{
    class EnvBuilder final : public Builder
    {
    public:
        EnvBuilder(const std::string& name)
            : _name(name)
        {
        }

        void add(TokenSequence& sequence) override
        {
            _arg.add(sequence);

            for(; !sequence.empty();)
            {
                const auto& token = sequence.top();
                switch (token.type)
                {
                    case END_ENV:
                    {
                        sequence.next();
                        return;
                    }

                    default:
                        _tableBuilder.add(sequence);
                        break;
                }
            }
        }

        std::string take() override
        {
            auto out = _tableBuilder.take();

            if (_name == "pmatrix")
            {
                out.insert(0, "<mfenced open='(' close=')'>");
                out.append("</mfenced>");

            }
            else if (_name == "bmatrix")
            {
                out.insert(0, "<mfenced open='[' close=']'>");
                out.append("</mfenced>");
            }
            else if (_name == "Bmatrix")
            {
                out.insert(0, "<mfenced open='{' close='}'>");
                out.append("</mfenced>");
            }
            else if (_name == "vmatrix")
            {
                out.insert(0, "<mfenced open='|' close='|'>");
                out.append("</mfenced>");
            }
            else if (_name == "Vmatrix")
            {
                out.insert(0, "<mfenced open='\xE2\x80\x96' close='\xE2\x80\x96'>");
                out.append("</mfenced>");
            }
            return out;
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

                std::size_t groupIndex = 0;
                for(bool finalize = false; !finalize && !sequence.empty(); sequence.next())
                {
                    const auto& token = sequence.top();
                    switch (token.type)
                    {
                        case START_GROUP:
                            ++groupIndex;
                            break;

                        case END_GROUP:
                            --groupIndex;
                            if (groupIndex == 0 && token.content[0] == '}') finalize = true;
                            break;

                        default:
                            break;
                    }
                    data += token.content;
                }
            }

        public:
            std::string data;
        };

    private:
        std::string _name;
        Arg _arg;
        TableBuilder _tableBuilder;
    };

    return std::make_unique<EnvBuilder>(name);
}

std::unique_ptr<Builder> makeSUM()
{
    return std::make_unique<SubSupBuilder>("<mo>\xE2\x88\x91</mo>", SubSupType::Limits);
}

std::unique_ptr<Builder> makePROD()
{
    return std::make_unique<SubSupBuilder>("<mo>\xE2\x88\x8F</mo>", SubSupType::Limits);
}

std::unique_ptr<Builder> makeINT()
{
    return std::make_unique<SubSupBuilder>("<mo>\xE2\x88\xAB</mo>", SubSupType::NoLimits);
}

std::unique_ptr<Builder> makeIINT()
{
    return std::make_unique<SubSupBuilder>("<mo>\xE2\x88\xAC</mo>", SubSupType::NoLimits);
}

std::unique_ptr<Builder> makeIIINT()
{
    return std::make_unique<SubSupBuilder>("<mo>\xE2\x88\xAD</mo>", SubSupType::NoLimits);
}

std::unique_ptr<Builder> makeIIIINT()
{
    return std::make_unique<SubSupBuilder>("<mo>\xE2\xA8\x8C</mo>", SubSupType::NoLimits);
}

std::unique_ptr<Builder> makeOINT()
{
    return std::make_unique<SubSupBuilder>("<mo>\xE2\x88\xAE</mo>", SubSupType::NoLimits);
}

std::unique_ptr<Builder> makeOIINT()
{
    return std::make_unique<SubSupBuilder>("<mo>\xE2\x88\xAF</mo>", SubSupType::NoLimits);
}

std::unique_ptr<Builder> makeOIIINT()
{
    return std::make_unique<SubSupBuilder>("<mo>\xE2\x88\xB0</mo>", SubSupType::NoLimits);
}

std::unique_ptr<Builder> makeLIM()
{
    return std::make_unique<SubSupBuilder>("<mi mathvariant=\"normal\">lim</mi>", SubSupType::Limits);
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

class AccentBuilder final : public Builder
{
public:
    AccentBuilder(const char* accent)
        : _accent(accent)
    {}

    void add(TokenSequence& sequence) override
    {
        _arg.add(sequence);
    }

    std::string take() override
    {
        std::string out(R"(<mover>)");
        out.append(_arg.take())
           .append(_accent)
           .append("</mover>");
        return out;
    }

private:
    const char* _accent;
    ArgBuilder _arg;
};

std::unique_ptr<Builder> makeBAR()
{
    return std::make_unique<AccentBuilder>("<mo>\xC2\xAF</mo>");
}

std::unique_ptr<Builder> makeDOT()
{
    return std::make_unique<AccentBuilder>("<mo>\x2E</mo>");
}

std::unique_ptr<Builder> makeDDOT()
{
    return std::make_unique<AccentBuilder>("<mo>\xC2\xA8</mo>");
}

std::unique_ptr<Builder> makeTILDE()
{
    return std::make_unique<AccentBuilder>("<mo stretchy=\"false\">\x7E</mo>");
}

std::unique_ptr<Builder> makeWIDETILDE()
{
    return std::make_unique<AccentBuilder>("<mo>\x7E</mo>");
}

std::unique_ptr<Builder> makeOVERLINE()
{
    return std::make_unique<AccentBuilder>("<mo>\xC2\xAF</mo>");
}

std::unique_ptr<Builder> makeVEC()
{
    return std::make_unique<AccentBuilder>("<mo>\xE2\x86\x92</mo>");
}

std::unique_ptr<Builder> makeWIDEHAT()
{
    return std::make_unique<AccentBuilder>("<mo>\x5E</mo>");
}

std::unique_ptr<Builder> makeUNDERLINE()
{
    class UNDERLINEBuilder final : public Builder
    {
    public:

        void add(TokenSequence& sequence) override
        {
            _arg.add(sequence);
        }

        std::string take() override
        {
            std::string out(R"(<munder>)");
            out.append(_arg.take())
               .append("<mo>\x5F</mo>")
               .append("</munder>");
            return out;
        }

    private:
        ArgBuilder _arg;
    };

    return std::make_unique<UNDERLINEBuilder>();
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

class SingleNodeBuilder final : public Builder
{
public:
    SingleNodeBuilder(std::string&& node)
        : _node(std::move(node))
    {
    }

    void add(TokenSequence&) override
    {
    }

    std::string take() override
    {
        return std::move(_node);
    }

private:
    std::string _node;
};

std::unique_ptr<Builder> makeQUAD()
{
    return std::make_unique<SingleNodeBuilder>("<mi>\xE2\x80\x81</mi>");
}

std::unique_ptr<Builder> makeQQUAD()
{
    return std::make_unique<SingleNodeBuilder>(R"(<mspace width="2em"/>)");
}

std::unique_ptr<Builder> makeTHICKSPACE()
{
    return std::make_unique<SingleNodeBuilder>(R"(<mspace width="0.27778em"/>)");
}

std::unique_ptr<Builder> makeMEDSPACE()
{
    return std::make_unique<SingleNodeBuilder>("<mi>\xE2\x81\x9F</mi>");
}

std::unique_ptr<Builder> makeTHINSPACE()
{
    return std::make_unique<SingleNodeBuilder>("<mi>\xE2\x80\x89</mi>");
}

std::unique_ptr<Builder> makeNEGSPACE()
{
    return std::make_unique<SingleNodeBuilder>(R"(<mspace width="-0.16667em"/>)");
}

std::unique_ptr<Builder> makeNEGMEDSPACE()
{
    return std::make_unique<SingleNodeBuilder>(R"(<mspace width="-0.22222em"/>)");
}

std::unique_ptr<Builder> makeNEGTHICKSPACE()
{
    return std::make_unique<SingleNodeBuilder>(R"(<mspace width="-0.27778em"/>)");
}

std::unique_ptr<Builder> makeSUBSTACK()
{
    class SUBSTACKBuilder final : public Builder
    {
        void add(TokenSequence& sequence) override
        {
            _arg.add(sequence);
        }

        std::string take() override
        {
            return _arg.take();
        }

    private:
        ArgTableBuilder _arg;
    };
    return std::make_unique<SUBSTACKBuilder>();
}

std::unique_ptr<Builder> makeMBOX()
{
    return std::make_unique<TextArgBuilder>(true);
}

std::unique_ptr<Builder> makeDISPLAYSTYLE()
{
    class DISPLAYSTYLEBuilder final : public Builder
    {
        void add(TokenSequence& sequence) override
        {
            _arg.add(sequence);
        }

        std::string take() override
        {
            std::string out(R"(<mstyle displaystyle="true">)");
            out.append(_arg.take())
               .append(R"(</mstyle>)");
            return out;
        }

    private:
        ArgBuilder _arg;
    };
    return std::make_unique<DISPLAYSTYLEBuilder>();
}

std::unique_ptr<Builder> makeTEXTSTYLE()
{
    class TEXTSTYLEBuilder final : public Builder
    {
        void add(TokenSequence& sequence) override
        {
            _arg.add(sequence);
        }

        std::string take() override
        {
            std::string out(R"(<mstyle displaystyle="false">)");
            out.append(_arg.take())
               .append(R"(</mstyle>)");
            return out;
        }

    private:
        ArgBuilder _arg;
    };
    return std::make_unique<TEXTSTYLEBuilder>();
}

std::unique_ptr<Builder> makePHANTOM()
{
    class PHANTOMBuilder final : public Builder
    {
        void add(TokenSequence& sequence) override
        {
            _arg.add(sequence);
        }

        std::string take() override
        {
            std::string out(R"(<mphantom>)");
            out.append(_arg.take())
               .append(R"(</mphantom>)");
            return out;
        }

    private:
        ArgBuilder _arg;
    };
    return std::make_unique<PHANTOMBuilder>();
}

const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()>& getBuilderFactory()
{
    static const std::unordered_map<std::string, std::unique_ptr<Builder>(*)()> map =
    {
        {" ", makeTHICKSPACE},
        {"!", makeNEGSPACE},
        {",", makeTHINSPACE},
        {":", makeMEDSPACE},
        {";", makeTHICKSPACE},
        {">", makeMEDSPACE},
        {"bar", makeBAR},
        {"binom", makeBINOM},
        {"cfrac", makeFRAC},
        {"closure", makeOVERLINE},
        {"dfrac", makeFRAC},
        {"displaystyle", makeDISPLAYSTYLE},
        {"dot", makeDOT},
        {"ddot", makeDDOT},
        {"frac", makeFRAC},
        {"genfrac", makeGENFRAC},
        {"hspace", makeHSPACE},
        {"lim", makeLIM},
        {"mathrm", makeMATHRM},
        {"mbox", makeMBOX},
        {"medspace", makeMEDSPACE},
        {"negmedspace", makeNEGMEDSPACE},
        {"negspace", makeNEGSPACE},
        {"negthickspace", makeNEGTHICKSPACE},
        {"negthinspace", makeNEGSPACE},
        {"overline", makeOVERLINE},
        {"overrightarrow", makeVEC},
        {"overset", makeOVERSET},
        {"phantom", makePHANTOM},
        {"prod", makePROD},
        {"product", makePROD},
        {"qquad", makeQQUAD},
        {"quad", makeQUAD},
        {"sqrt", makeSQRT},
        {"stackrel", makeOVERSET},
        {"substack", makeSUBSTACK},
        {"sum", makeSUM},
        {"tbinom", makeBINOM},
        {"textstyle", makeTEXTSTYLE},
        {"tfrac", makeFRAC},
        {"thickspace", makeTHICKSPACE},
        {"thinspace", makeTHINSPACE},
        {"tilde", makeTILDE},
        {"underline", makeUNDERLINE},
        {"underset", makeUNDERSET},
        {"vec", makeVEC},
        {"widebar", makeOVERLINE},
        {"widehat", makeWIDEHAT},
        {"widetilde", makeWIDETILDE},
        {"widevec", makeVEC},
        {"~", makeTILDE},
        {"int", makeINT},
        {"integral", makeINT},
        {"iiiint", makeIIIINT},
        {"iiint", makeIIINT},
        {"iint", makeIINT},
        {"oiiint", makeOIIINT},
        {"oiint", makeOIINT},
        {"oint", makeOINT},
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
         << R"(<math xmlns="http://www.w3.org/1998/Math/MathML">)" << std::endl;

    TokenSequence sequence{lexer};
    RowBuilder builder;
    while(!sequence.empty()) builder.add(sequence);

    _out << builder.take() << std::endl
         << R"(</math>)" << std::endl;
}
} // namespace TXL
