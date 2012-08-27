
dat = read.csv(fname.txt, sep=';')

dat.seq = subset(dat, cores == 1)
dat.seq.Ti = aggregate(dat.seq$duration, list(wkload=dat.seq$wkload), FUN=mean)
dat.par.maxcores = aggregate(dat$cores, list(wkload=dat$wkload), FUN=max)
# :-/
dat.par = subset(dat, cores==8)
dat.par.Ti = aggregate(dat.par$duration, list(wkload=dat.par$wkload), FUN=mean)
alphas = (dat.par.maxcores$x * (dat.par.Ti$x / dat.seq.Ti$x - 1)) / (1.0 - dat.par.maxcores$x)

wkparams = data.frame(wkload=levels(dat$wkload), Tseq=dat.seq.Ti$x, alpha=alphas)


A00.Tseq = subset(wkparams, wkload=='A00')$Tseq
A05.Tseq = subset(wkparams, wkload=='A05')$Tseq
A10.Tseq = subset(wkparams, wkload=='A10')$Tseq
A00.alpha = subset(wkparams, wkload=='A00')$alpha
A05.alpha = subset(wkparams, wkload=='A05')$alpha
A10.alpha = subset(wkparams, wkload=='A10')$alpha

pp.Tseq = c(A00.Tseq, A05.Tseq, A10.Tseq)
pp.alpha = c(A00.alpha, A05.alpha, A10.alpha)

LAT_NCORES = 12
latopts = data.frame(cores=rep(NA,LAT_NCORES**3), p1=rep(NA, LAT_NCORES**3), p2=rep(NA, LAT_NCORES**3), p3=rep(NA, LAT_NCORES**3), tmax=rep(NA,LAT_NCORES**3))
i = 1
for (p1 in seq(1, LAT_NCORES)) {
    for (p2 in seq(1, LAT_NCORES)) {
        for (p3 in seq(1, LAT_NCORES)) {
            p = c(p1,p2,p3)
            t = pp.Tseq * (pp.alpha / p + 1 - pp.alpha)
            latopts[i, ] = c(sum(p), p, sum(t))
            i = i + 1
        }
    }
}

best.latopts = data.frame(cores=seq(3,LAT_NCORES), p1=rep(NA, LAT_NCORES-2), p2=rep(NA, LAT_NCORES-2), p3=rep(NA, LAT_NCORES-2), tmax=rep(NA,LAT_NCORES-2))
for (C in seq(3,LAT_NCORES)) {
    best.latopts[C-2,] = subset(latopts, tmax==min(subset(latopts, cores<=C)$tmax))
}

# we'll go with the option 3
pp.p = best.latopts[6, 2:4]
# latency
pp.t = pp.Tseq * (pp.alpha / pp.p + 1 - pp.alpha)

MAX_R = 16
# individual throuputs (wi) for different replications (r)
w.ind = data.frame(r=seq(1,MAX_R), w1=(seq(1,MAX_R) / rep(as.real(pp.t[1,1]), times=MAX_R)), w2=(seq(1,MAX_R) / rep(as.real(pp.t[1,2]), times=MAX_R)), 
      w3=(seq(1,MAX_R) / rep(as.real(pp.t[1,3]), times=MAX_R)) )

w = data.frame(r1=seq(1,MAX_R), r2=rep(NA,MAX_R), r3=rep(NA,MAX_R), w1=rep(NA,MAX_R), w2=rep(NA,MAX_R), w3=rep(NA,MAX_R), cores=rep(NA,MAX_R))

# here we assume that box 1 is the most sequential one.
for (r1 in seq(1, MAX_R)) {
  w1x = w.ind[r1,'w1']
  # w2: choose the closest higher
  w2x = min(subset(w.ind, w2>=w1x)$w2)
  w3x = min(subset(w.ind, w3>=w2x)$w3)
  #
  r2 = w.ind[which(w2x == w.ind$w2),]$r
  r3 = w.ind[which(w3x == w.ind$w3),]$r
  #
  cores = sum(c(r1,r2,r3) * pp.p)
  w[r1,] = c(r1, r2, r3, w1x, w2x, w3x, cores)
}



