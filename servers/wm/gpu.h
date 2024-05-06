#include <reimu/core/result.h>
#include <reimu/os/error.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <fcntl.h>
#include <unistd.h>

#define MAX_CARD_NUM 5

using reimu::Result;

class DRMError : public reimu::ErrorBase {
    std::string as_string() const override {
        return "DRM error";
    }
};

class GPU {
public:
    static Result<GPU*, reimu::ErrorBox> open_first_gpu() {
        int card_num = 0;
        int card;

        std::string path = "/dev/dri/card";
        while (card_num < MAX_CARD_NUM) {
            card = open((path + std::to_string(card_num)).c_str(), O_RDWR | O_NONBLOCK);

            // Case 1: success!
            if (card >= 0) {
                break;
            }

            // Case 2: unexpected error
            if (errno != ENOENT) {
                return ERR(reimu::OSError{ errno });
            }

            // Case 3: card does not exist
            card_num++;
        }

        if (card < 0) {
            // Failed to find DRI device
            return ERR(reimu::OSError{ errno });
        }

        drmModeRes *res = drmModeGetResources(card);
        if (!res) {
            return ERR(DRMError{});
        }        

        int num_conn = res->count_connectors;
        for (int i = 0; i < num_conn; i++) {
            drmModeConnector *conn = drmModeGetConnector(card, res->connectors[i]);
            if (!conn) continue;

            printf("Connector type: ", conn->connector_type);
            int num_modes = conn->count_modes;
            for (int i = 0; i < num_modes; i++) {
                drmModeModeInfo *mode = conn->modes + i;

                printf("\tFound mode: %s\n", mode->name);
            }

            drmModeFreeConnector(conn);
        }

        return OK(new GPU(card));
    }

private:
    GPU(int card)
        : m_card(card) {

    }

    int m_card;
};
