// This file is part of RBDyn.
//
// RBDyn is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RBDyn is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with RBDyn.  If not, see <http://www.gnu.org/licenses/>.

// associated header
#include "ID.h"

// includes
// RBDyn
#include "MultiBody.h"
#include "MultiBodyConfig.h"

namespace rbd
{

InverseDynamics::InverseDynamics(const MultiBody& mb):
	f_(mb.nrBodies())
{
}

void InverseDynamics::inverseDynamics(const MultiBody& mb, MultiBodyConfig& mbc)
{
	const std::vector<Body>& bodies = mb.bodies();
	const std::vector<Joint>& joints = mb.joints();
	const std::vector<int>& pred = mb.predecessors();

	sva::MotionVec a_0(Eigen::Vector3d::Zero(), mbc.gravity);

	for(std::size_t i = 0; i < bodies.size(); ++i)
	{
		const sva::PTransform& X_i = mbc.jointConfig[i];
		const sva::PTransform& X_p_i = mbc.parentToSon[i];

		const sva::MotionVec& vj_i = mbc.jointVelocity[i];
		sva::MotionVec ai_tan = X_i*joints[i].tanAccel(mbc.alphaD[i]);

		const sva::MotionVec& vb_i = mbc.bodyVelB[i];

		if(pred[i] != -1)
			mbc.bodyAccB[i] = X_p_i*mbc.bodyAccB[pred[i]] + ai_tan + vb_i.cross(vj_i);
		else
			mbc.bodyAccB[i] = a_0 + ai_tan + vb_i.cross(vj_i);

		f_[i] = bodies[i].inertia()*mbc.bodyAccB[i] +
			vb_i.crossDual(bodies[i].inertia()*vb_i) -
			mbc.bodyPosW[i].dualMul(mbc.force[i]);
	}

	for(int i = static_cast<int>(bodies.size()) - 1; i >= 0; --i)
	{
		for(int j = 0; j < joints[i].dof(); ++j)
		{
			mbc.jointTorque[i][j] = mbc.motionSubspace[i].col(j).transpose()*
				f_[i].vector();
		}

		if(pred[i] != -1)
		{
			const sva::PTransform& X_p_i = mbc.parentToSon[i];
			f_[pred[i]] = f_[pred[i]] + X_p_i.transMul(f_[i]);
		}
	}
}

} // namespace rbd