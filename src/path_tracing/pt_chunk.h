

#include "pt_voxel.h"
#include <iostream>
#include <vector>

namespace PT {
	class Chunk {
	public:
		Voxel * * * m_pBlocks;
		Chunk() {
			m_pBlocks = new Voxel**[16];
			for (int i = 0; i < 16; i++) {
				m_pBlocks[i] = new Voxel*[16];
				for (int j = 0; j < 16; j++) {
					m_pBlocks[i][j] = new Voxel[16];
				}
			}
		}

		float * flatten(){
			float * vox = new float[4096];
			for (int i = 0; i < 4096; i++) {
				vox[i] = 1.0f;
			}
			return vox;
			delete[] vox;
		}

		/*
		void flatten() {
			for (int i = 0; i < 16; i++) {
				for (int j = 0; j < 16; j++) {
					for (int k = 0; k < 16; k++) {
						world[i][j][k] = 1.0f;
					}
				}
			}
			
		}
		*/

		~Chunk() { // Delete the blocks
			for (int i = 0; i < 16; ++i) {
				for (int j = 0; j < 16; ++j) {
					delete[] m_pBlocks[i][j];
				}
				delete[] m_pBlocks[i];
			}
			delete[] m_pBlocks;
			
		}
	};
}