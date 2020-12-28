// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

#include <bignum.h>
#include <chainparams.h>

// mmocoin: find last block index up to pindex
const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake)
{
    while (pindex && pindex->pprev && (pindex->IsProofOfStake() != fProofOfStake))
        pindex = pindex->pprev;
    return pindex;
}

unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, bool fProofOfStake, const Consensus::Params& params)
{
	int ForkingPoint = 1035000;
	int ForknStakeTargetSpacing = 300;
    if (pindexLast == nullptr)
        return UintToArith256(params.powLimit).GetCompact(); // genesis block

    if (pindexLast->nHeight < 1001)
        return UintToArith256(params.powLimit).GetCompact();

    const CBlockIndex* pindexPrev = GetLastBlockIndex(pindexLast, fProofOfStake);
    if (pindexPrev->pprev == nullptr)
        return UintToArith256(params.bnInitialHashTarget).GetCompact(); // first block
    const CBlockIndex* pindexPrevPrev = GetLastBlockIndex(pindexPrev->pprev, fProofOfStake);
    if (pindexPrevPrev->pprev == nullptr)
        return UintToArith256(params.bnInitialHashTarget).GetCompact(); // second block

    int64_t nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime();
	
	if (pindexLast-> nHeight < ForkingPoint) {
		if (nActualSpacing < 0)
			nActualSpacing = params.nStakeTargetSpacing;
	} else {
		if (nActualSpacing < 0)
			nActualSpacing = (ForknStakeTargetSpacing);
	}

    // mmocoin: target change every block
    // mmocoin: retarget with exponential moving toward target spacing
    CBigNum bnNew;
	if (pindexLast-> nHeight < ForkingPoint) {
		bnNew.SetCompact(pindexPrev->nBits);
		int64_t nInterval = params.nTargetTimespan / params.nStakeTargetSpacing;
		bnNew *= ((nInterval - 1) * params.nStakeTargetSpacing + nActualSpacing + nActualSpacing);
		bnNew /= ((nInterval + 1) * params.nStakeTargetSpacing);
	} else {
		bnNew.SetCompact(pindexPrev->nBits);
		int64_t nInterval = params.nTargetTimespan / (ForknStakeTargetSpacing);
		bnNew *= ((nInterval - 1) * (ForknStakeTargetSpacing) + nActualSpacing + nActualSpacing);
		bnNew /= ((nInterval + 1) * (ForknStakeTargetSpacing));
	}

    if (bnNew > CBigNum(params.powLimit))
        bnNew = CBigNum(params.powLimit);

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
