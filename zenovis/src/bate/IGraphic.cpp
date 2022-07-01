#include <zenovis/Scene.h>
#include <zeno/core/IObject.h>
#include <zeno/utils/cppdemangle.h>
#include <zeno/utils/log.h>
#include <zenovis/bate/IGraphic.h>

namespace zenovis {

std::unique_ptr<IGraphic> makeGraphic(Scene *scene, zeno::IObject *obj) {
    MakeGraphicVisitor visitor;
    visitor.in_scene = scene;

    if (0) {
#define _ZENO_PER_XMACRO(TypeName, ...) \
    } else if (auto p = dynamic_cast<zeno::TypeName *>(obj)) { \
        visitor.visit(p); \
ZENO_XMACRO_IObject(_ZENO_PER_XMACRO)
#undef _ZENO_PER_XMACRO
    }

    auto res = std::move(visitor.out_result);

    if (!res)
        zeno::log_debug("load_object: unexpected view object {}",
                        zeno::cppdemangle(typeid(*obj)));

    //printf("%s\n", ext.c_str());
    //assert(0 && "bad file extension name");
    return res;
}

} // namespace zenovis
