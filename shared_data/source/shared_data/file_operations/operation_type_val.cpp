#include <shared_data/file_operations/operation_type.hpp>

namespace SharedData
{
    void to_val(Nui::val& v, OperationType const& operationType)
    {
        switch (operationType)
        {
            case OperationType::Download:
                v = Nui::val::u8string("Download");
                break;
            case OperationType::Upload:
                v = Nui::val::u8string("Upload");
                break;
            case OperationType::Rename:
                v = Nui::val::u8string("Rename");
                break;
            case OperationType::Delete:
                v = Nui::val::u8string("Delete");
                break;
            case OperationType::Scan:
                v = Nui::val::u8string("Scan");
                break;
            case OperationType::BulkDownload:
                v = Nui::val::u8string("BulkDownload");
                break;
        }
    }
    void from_val(Nui::val const& v, OperationType& operationType)
    {
        auto const str = v.template as<std::string>();
        if (str == "Download")
            operationType = OperationType::Download;
        else if (str == "Upload")
            operationType = OperationType::Upload;
        else if (str == "Rename")
            operationType = OperationType::Rename;
        else if (str == "Delete")
            operationType = OperationType::Delete;
        else if (str == "Scan")
            operationType = OperationType::Scan;
        else if (str == "BulkDownload")
            operationType = OperationType::BulkDownload;
        else
            throw std::runtime_error("Invalid OperationType string: " + str);
    }
}