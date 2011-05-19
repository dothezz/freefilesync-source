#include "i18n.h"
#include <memory>

using namespace zen;

namespace
{
std::auto_ptr<TranslationHandler> globalHandler;
}

void zen::setTranslator(TranslationHandler* newHandler)
{
    globalHandler.reset(newHandler);
}


TranslationHandler* zen::getTranslator()
{
    return globalHandler.get();
}
