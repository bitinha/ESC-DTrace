#!/usr/sbin/dtrace -qs



syscall::lwp_park:entry
/pid==$target/
{
	self->time = timestamp;
}

syscall::lwp_park:return
/self->time/
{
	self->time = timestamp - self->time;
	@tempo["park"] = llquantize(self->time,10,3,5,10);
	@ttime["park"] = sum(self->time);
	self->time = 0;
}

fbt::lwp_unpark:entry
/pid==$target/
{
	self->time = timestamp;
}

fbt::lwp_unpark:return
/self->time/
{
	self->time = timestamp - self->time;
	@ttime["unpark"] = sum(self->time);
	@tempo["unpark"] = llquantize(self->time,10,3,5,10);
	self->time=0;
}

/*
syscall::dt:return
/pid == $target && errno != 0/
{
	globalErrors++;
	@errors[probefunc] = count();
}
*/
