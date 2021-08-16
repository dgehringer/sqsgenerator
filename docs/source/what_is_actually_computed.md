# How does `sqsgenerator` work?
## Pair Short-Range-Order

### What does `sqsgenerator` actually compute?

`sqsgenerator` computes so-called *Warren-Cowley* short-range-order (WC-SRO) parameters, to quantify the "*randomness*" of an atomistic model. Thus consider a binary alloy with just $A$ and $B$ atoms, then the  SRO parameter is defined as

$$
\alpha_{AB} = 1 - \dfrac{N_{AB}}{NMx_Ax_B}
$$ (eqn:wc-sro)

Where $N_{AB}$ the number of $A-B$ bonds, $N$  the number of atoms in the cell, $x_A$ ($x_B$) the mole fraction of $A$ ($B$) and $M$ the number of neighbors on the lattice (e. g $M^{fcc}=12$). Thus it compares the number of $A-B$ bonds in a given (numerator) configuration with that of statistical ideal solid solution (denominator). Thus the following cases can occur

* $\alpha_{AB} < 0$: system tends to be **ordered**
* $\alpha_{AB} > 0$: system tends to be **clustered**
* $\alpha_{AB} \approx 0$: system tends to be **disordered**

Therefore `sqsgenerator` tries to optimize a given system such that the SRO approaches zero. 

### Can `sqsgenerator` handle also second nearest neighbors?

Yes it actually can! Indeed on a *fcc* lattice each atom exhibits $M^{fcc}_1=12$ nearest neighbors, but there are also  $M^{fcc}_2=6$ second nearest neighbors and even  $M^{fcc}_3=24$ third nearest neighbors. However, it is very easy to generalize the *Warren-Cowley* SRO to more "*coordination shells*" which, therefore becomes a vector

$$
\alpha^k_{AB} = 1 - \dfrac{N_{AB}^k}{NM^kx_Ax_B}.
$$ (eqn:wc-sro-shells)

### But what about optimizing multi-component alloys?

Similarly the generalization to multi-component systems is a straight as for the the coordination shells. Consider a system of $\{A,B,C\}$ atoms. Now we have to also consider $A-C$ and $B-C$ bonds, thus resulting in 6 SRO parameters for each coordination shells (also $A-A$, $B-B$ and $C-C$ bonds are considered), thus the initial parameter becomes a three dimensional object

$$
\alpha^i_{\xi\eta} = 1 - \dfrac{N^i_{\xi\eta}}{NM^ix_{\xi}x_{\eta}}
$$ (eqn:wc-sro-multi)

where again $M^{i}$ is the number of sites in the $i$-th coordination shell, $x_\xi$ and $x_\eta$ are the mole-fraction of the corresponding species and $N^i_{\xi\eta}$ the number of $\xi-\eta$ "*bonds*"/pairs in the coordination shell.

### Putting it all together!

This is a quite large number of parameters to optimize. In order to squeeze those together we produce a single scalar value by simply summing over those parameters. Therefore we define our "*objective-function*" as for a certain atomic configuration $\sigma$ as,

$$
\mathcal{O}(\sigma) = \sum_{k}w^i\sum_{\xi,\eta} \left| \alpha'_{\xi\eta}-p_{\xi\eta}\alpha^i_{\xi\eta} \right|
$$ (eqn:objective)

where $w^i$ are the coordination shell weighting factor, to take into account the decreasing influence of shells which are farther away.  $\alpha'_{\xi\eta}$ are the "*target objective*" parameters. Those can be used to optimize pairs between different species differently.
$p_{\xi\eta}$ are *pair weights* and allows the user to weight pair types differently.

The `sqsgenerator` tries different atomic configurations $\sigma$ by "*randomly shuffling*" or "*systematically*" (in lexicographical order) to filter out those configuration which **minimize** $\mathcal{O}(\sigma)$.


```{note}
The mapping of an atomic configuration $\sigma$ on a scalar values via $\mathcal{O}(\sigma)$ is non-injective, thus you might end up with quite a lot of configurations, which are "*equal*" in the here presented formalism.
```

```{note}
In the technical part of the documentation we refer to $w^i$ as the `shell_weights` and to  $\alpha'_{\xi\eta}$  as the `target_objective`.
```

## Triplet-based Short-Range-Order

The objective-function above is a non-injective function, meaning, that one can end up with a lot of structures which are the same in the formalism presented above, therefore it might be beneficial to further investigate those structures, by not counting pairs, but rather triplets.

### the Pair-Shell-Matrix

To be able to generalize Warren-Cowley parameters to triplets we introduce a "*pair-shell-matrix*" $\mathbf{c}$ of dimensions ($N\times N$) where $N$ is the number of atoms in the system under consideration. Each entry $c_{ij}$ denotes the "*coordination-shell*" between lattice site $i$ and lattice site $j$.

### Number of triplets for a tuple of coordination shell

Similarly as above, when we generalized Warren-Cowley SRO parameter to an arbitrary amount of coordination shells, we can do the same for triplets. However, a pair of lattice sites is uniquely characterized by a **single** distance/coordination, we need **three** distances/coordination-shells  to properly describe a triplet of lattice positions.
E. g all triplets that can be formed from **first**, **second** and **third** nearest neighbors will be identified by $\{1,2,3\}$. Let $i$,$j$ and $k$ be coordination shells, then $\{i,j,k\}$​ represents the set of a tuples that can be generated upon permuting the sequence. In other words  $\{i,j,k\} = \{(i,j,k), (i,k,j),\ldots\}$. Furthermore let $\widetilde{M}$ be the set of all lattice positions with a sphere with radius $\max(i,j,k)$.

It follows that the number of triplets that can be formed $T^{\left\{i,j,k\right\}}$ reads as

$$
T^{\left\{i,j,k\right\}} = \left|\left\{ (\xi,\eta,\zeta) | (c_{\xi\eta}, c_{\eta\zeta},c_{\xi\zeta}) \in \left\{i,j,k\right\} \right\} \right|_{\forall \xi,\eta,\zeta \in \widetilde{M}}
$$ (eqn:triplet-number)

The number of all triplets are computed from the input structure before the iteration itself starts.
$$
\beta^{ijk}_{\xi\eta\zeta} = 1 - \dfrac{N^{ijk}_{\xi\eta\zeta}}{\frac{1}{|\{i,j,k\}|}NT^{\left\{i,j,k\right\}}x_\xi x_\eta x_\zeta}
$$ (eqn:triplet-sro)

