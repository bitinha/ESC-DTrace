#!/usr/sbin/dtrace -qs

#pragma D option quiet

BEGIN
{
	processos[$target] = 1;
}


proc:::create
/processos[pid]/
{
	print(args[0]->pr_pid);
	processos[args[0]->pr_pid] = 1;
}



profile-997
/*processos[args[0]->pr_pid]*/
{
	estado = (processos[pid]==1) ? curlwpsinfo->pr_state : 0;
	@name[curlwpsinfo->pr_sname] = count();
	@proc = lquantize(estado, 0, 10, 1);
}


