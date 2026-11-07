static status_t
Status(IntelApertureLeaseState::Result result)
{
	switch (result) {
		case IntelApertureLeaseState::Success: return B_OK;
		case IntelApertureLeaseState::Busy: return B_BUSY;
		case IntelApertureLeaseState::Exhausted: return B_NO_MEMORY;
		default: return B_BAD_VALUE;
	}
}

// A ticket is ended under the owner mutex on every return path. Backend calls
// occur without this mutex; the pin and permanent module retain their targets.
class Operation {
public:
	Operation(uint64 lease, IntelApertureLeaseState::Access access)
		: fCanRetainBacking(access == IntelApertureLeaseState::Allocate
			|| access == IntelApertureLeaseState::Free),
		  fFailed(true)
	{
		mutex_lock(&sLock);
		fStatus = Status(sState.BeginOperation(lease, access, fTicket));
		mutex_unlock(&sLock);
	}
	~Operation()
	{
		if (fStatus != B_OK)
			return;
		mutex_lock(&sLock);
		sState.EndOperation(fTicket, fCanRetainBacking && fFailed);
		mutex_unlock(&sLock);
	}
	status_t InitCheck() const { return fStatus; }
	void Succeeded() { fFailed = false; }
private:
	Operation(const Operation&) = delete;
	Operation& operator=(const Operation&) = delete;
	IntelApertureLeaseState::Operation fTicket;
	status_t fStatus;
	bool fCanRetainBacking;
	bool fFailed;
};

static status_t
Query(uint64 lease, aperture_info* info)
{
	Operation operation(lease, IntelApertureLeaseState::Query);
	if (operation.InitCheck() != B_OK)
		return operation.InitCheck();
	if (info == NULL)
		return B_BAD_VALUE;
	return sGART->get_aperture_info(sAperture, info);
}

static status_t
Allocate(uint64 lease, size_t size, size_t alignment, uint32 flags,
	addr_t* base, phys_addr_t* physical)
{
	Operation operation(lease, IntelApertureLeaseState::Allocate);
	if (operation.InitCheck() != B_OK)
		return operation.InitCheck();
	status_t status = sGART->allocate_memory(sAperture, size, alignment,
		flags, base, physical);
	if (status == B_OK)
		operation.Succeeded();
	return status;
}

static status_t
Free(uint64 lease, addr_t base)
{
	Operation operation(lease, IntelApertureLeaseState::Free);
	if (operation.InitCheck() != B_OK)
		return operation.InitCheck();
	status_t status = sGART->free_memory(sAperture, base);
	if (status == B_OK)
		operation.Succeeded();
	return status;
}

static status_t
FinishDisplay(uint64 lease, bool initialized)
{
	mutex_lock(&sLock);
	status_t result = Status(sState.FinishInitialization(lease, initialized));
	mutex_unlock(&sLock);
	return result;
}

static status_t
ReleaseDisplay(uint64 lease)
{
	mutex_lock(&sLock);
	IntelApertureLeaseState::Result release = sState.Release(lease);
	if (release == IntelApertureLeaseState::Busy)
		release = sState.AbortInitialization(lease);
	status_t result = Status(release);
	mutex_unlock(&sLock);
	// Success means lease revocation, including aborted initialization.
	// Existing operation tickets still retain their pins until completion.
	// Last close is not scanout-idle evidence. Keep MMIO, GART and backing.
	return result;
}

