#include <shared_data/file_operations/operation_added.hpp>

namespace SharedData
{
    void to_val(Nui::val& v, OperationAdded const& operationAdded)
    {
        v = Nui::val::object();
        Nui::val operationIdVal;
        to_val(operationIdVal, operationAdded.operationId);
        v.set("operationId", operationIdVal);

        Nui::val typeVal;
        to_val(typeVal, operationAdded.type);
        v.set("type", typeVal);
    }
    void from_val(Nui::val const& v, OperationAdded& operationAdded)
    {
        from_val(v["operationId"], operationAdded.operationId);
        from_val(v["type"], operationAdded.type);
    }
}