#include "tool.h"

Tool::Tool(const QString& name, const QString& description)
    : m_name(name), m_description(description)
{
}

Tool::~Tool()
{
}

QString Tool::name() const
{
    return m_name;
}

QString Tool::description() const
{
    return m_description;
}

void Tool::addParameter(const QString& name, const QString& type, const QString& description, bool required, const QString& defaultValue)
{
    Parameter param;
    param.name = name;
    param.type = type;
    param.description = description;
    param.required = required;
    param.defaultValue = defaultValue;
    m_parameters.append(param);
}

QList<Parameter> Tool::parameters() const
{
    return m_parameters;
}

QString Tool::formatForPrompt() const
{
    QString result = "- " + m_name + ": " + m_description + "\n";
    result += "  Parameters:\n";
    
    for (const Parameter& param : m_parameters) {
        QString requiredStr = param.required ? "[required]" : "[optional]";
        QString defaultStr = !param.defaultValue.isEmpty() ? " (default: " + param.defaultValue + ")" : "";
        result += "    - " + param.name + " (" + param.type + ") " + requiredStr + ": " + param.description + defaultStr + "\n";
    }
    
    return result;
}
