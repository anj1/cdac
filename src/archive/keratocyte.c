
/* an implementation of the keratocyte motion model developed by Mogilner et al
 * in 2001 */

/* TODO */
#define M_MAX 1.0

struct mogilner_s{
	double kbt,delta,fr,v0,vm,vt,m,gamma,kappa,eta,ka,kd,b1,b2,i,l,d;
};

/* units: micrometers, piconewtons, seconds */
const struct mogilner_s mogilner1 = {
	.kbt 	= 0.0041,
	.delta 	= 0.0027,
	.fr 	= 2000,
	.v0 	= 0.2,
	.vm 	= 0.5,
	.vt 	= 0.2,
	.m 		= 10000,
	.gamma 	= 0.02,
	.eta 	= 0.04,
	.ka 	= 0.01,
	.kd 	= 0.01,
	.b1 	= 1.0,
	.b2		= 1.0,
	.i 		= 1000,
	.l 		= 5.0,
	.d 		= 0.03
};

/* m: myosin [Nx2]
 * v: rate of myosin drift at each point [Nx2]
 * L: width of lamellipod
 * ln: length of network actin [Nx2]
 * lb: length of bundled actin [Nx2]
 * iv: ventral density of integrin molecules
 * id: dorsal  density of integrin molecules
 * vp: velocity of protrusion
 */
void update_actomyosin(struct mogilner_s *k, double *m, double *v, double L, double *ln, double *lb, double *iv, double *id, double vp, int n,double dx, double dt)
{
	int i;
	double mbar;
	double Mbar;
	double alpha;
	
	double dm,dln,dlb,dv,div,did;
	
	/* TODO: the only thing I don't understand... */
	Mbar = k->m;
	
	/* find myosin density */
	for(i=0;i<n;i++)
		mbar += m[i];
	mbar = (Mbar - mbar)/k->l;
	
	/* update myosin clusters */
	for(i=1;i<n-1;i++){
		
			/* find relative area covered by myosin clusters */
		alpha = 1.0;
		if(m[i] < M_MAX) alpha = m[i] / M_MAX;
	
		dm  = k->ka*mbar - k->kd*m[i] - vp*(m[i]-m[i-1])/dx + ((v[i]-v[i-1])*m[i] + (m[i]-m[i-1])*v[i])/dx;
		dln = -k->gamma*ln[i] + ( alpha*v[i] - vp)*(ln[i]-ln[i-1])/dx;
		dlb = -k->gamma*lb[i] - vp*(lb[i]-lb[i-1])/dx - alpha*v[i]*(ln[i]-ln[i-1])/dx + ((v[i]-v[i-1])*lb[i] + (lb[i]-lb[i-1])*v[i])/dx;
		
		dv  = k->vm*(1 - ((1 - alpha)/m[i])*(k->b1*ln[i] + k->b2*iv[i]));
		
		div = -vp*(iv[i]-iv[i-1])/dx - k->kappa*iv[i];
		did = k->d*(2*id[i] - id[i-1] - id[i+1])/(dx*dx) + (k->vt - vp)*(id[i]-id[i-1])/dx + k->kappa*iv[i];
		
		m [n+i] = m [i] + dt*dm;
		ln[n+i] = ln[i] + dt*dln;
		lb[n+i] = lb[i] + dt*dlb;
		v [n+i] = v [i] + dt*dv;
		iv[n+i] = iv[i] + dt*div;
		id[n+i] = id[i] + dt*did;
		
	}
}

void init_actomyosin(double *m, int n)
{
	int i;
	
	for(i=0;i<n;i++){
		
	}
}

int main()
{
	return 0;
}
