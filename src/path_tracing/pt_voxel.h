#include "pt_blocktypes.h"


namespace PT {

	class Voxel {
	public:
		bool m_active;
		BlockType type;
		Voxel() {
			m_active = true;
			type = BlockType::BlockType_Default;
		}

		bool isActive() {
			return m_active;
		}

		void setActive(bool active) {
			m_active = active;
		}

		~Voxel() {}
	};
}
