/*****************************************************************************
* Model: dpp.qm
* File:  ./table.c
*
* This code has been generated by QM tool (see state-machine.com/qm).
* DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*****************************************************************************/
/* @(/2/2) .................................................................*/
#include "qpn_port.h"
#include "dpp.h"
#include "bsp.h"

Q_DEFINE_THIS_MODULE("table")

/* Active object class -----------------------------------------------------*/
/* @(/1/1) .................................................................*/
typedef struct TableTag {
/* protected: */
    QActive super;

/* private: */
    uint8_t fork[N_PHILO];
    uint8_t isHungry[N_PHILO];
} Table;

/* protected: */
static QState Table_initial(Table * const me);
static QState Table_active(Table * const me);
static QState Table_serving(Table * const me);
static QState Table_paused(Table * const me);


/* Global objects ----------------------------------------------------------*/
/* @(/1/7) .................................................................*/
struct TableTag AO_Table;


#define RIGHT(n_) ((uint8_t)(((n_) + (N_PHILO - 1U)) % N_PHILO))
#define LEFT(n_)  ((uint8_t)(((n_) + 1U) % N_PHILO))
#define FREE      ((uint8_t)0)
#define USED      ((uint8_t)1)

/*..........................................................................*/
/* @(/1/9) .................................................................*/
void Table_ctor(void) {
    uint8_t n;
    Table *me = &AO_Table;

    QActive_ctor(&me->super, Q_STATE_CAST(&Table_initial));
    for (n = 0U; n < N_PHILO; ++n) {
        me->fork[n] = FREE;
        me->isHungry[n] = 0U;
    }
}
/* @(/1/1) .................................................................*/
/* @(/1/1/2) ...............................................................*/
/* @(/1/1/2/0) */
static QState Table_initial(Table * const me) {
    uint8_t n;
    for (n = 0U; n < N_PHILO; ++n) {
        me->fork[n] = FREE;
        me->isHungry[n] = 0U;
        BSP_displayPhilStat(n, "thinking");
    }
    return Q_TRAN(&Table_serving);
}
/* @(/1/1/2/1) .............................................................*/
static QState Table_active(Table * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* @(/1/1/2/1/0) */
        case TERMINATE_SIG: {
            BSP_terminate(0);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/1/2/1/1) */
        case EAT_SIG: {
            Q_ERROR();
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/* @(/1/1/2/1/2) ...........................................................*/
static QState Table_serving(Table * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* @(/1/1/2/1/2) */
        case Q_ENTRY_SIG: {
            uint8_t n;
            for (n = 0U; n < N_PHILO; ++n) { /* give permissions to eat... */
                if ((me->isHungry[n] != 0U)
                    && (me->fork[LEFT(n)] == FREE)
                    && (me->fork[n] == FREE))
                {
                    QActive *philo;

                    me->fork[LEFT(n)] = USED;
                    me->fork[n] = USED;
                    philo =
                       (QActive *)Q_ROM_PTR(QF_active[PHILO_0_PRIO + n].act);
                    QACTIVE_POST(philo, EAT_SIG, n);
                    me->isHungry[n] = 0U;
                    BSP_displayPhilStat(n, "eating  ");
                }
            }
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/1/2/1/2/0) */
        case HUNGRY_SIG: {
            uint8_t n, m;
            QActive *philo;

            n = (uint8_t)(Q_PAR(me) - PHILO_0_PRIO);
            /* phil ID must be in range and he must be not hungry */
            Q_ASSERT((n < N_PHILO) && (me->isHungry[n] == 0U));

            BSP_displayPhilStat(n, "hungry  ");
            m = LEFT(n);
            /* @(/1/1/2/1/2/0/0) */
            if ((me->fork[m] == FREE) && (me->fork[n] == FREE)) {
                me->fork[m] = USED;
                me->fork[n] = USED;
                philo = (QActive *)Q_ROM_PTR(QF_active[PHILO_0_PRIO + n].act);
                QACTIVE_POST(philo, EAT_SIG, n);
                BSP_displayPhilStat(n, "eating  ");
                status_ = Q_HANDLED();
            }
            /* @(/1/1/2/1/2/0/1) */
            else {
                me->isHungry[n] = 1U;
                status_ = Q_HANDLED();
            }
            break;
        }
        /* @(/1/1/2/1/2/1) */
        case DONE_SIG: {
            uint8_t n, m;
            QActive *philo;

            n = (uint8_t)(Q_PAR(me) - PHILO_0_PRIO);
            /* phil ID must be in range and he must be not hungry */
            Q_ASSERT((n < N_PHILO) && (me->isHungry[n] == 0U));

            BSP_displayPhilStat(n, "thinking");
            m = LEFT(n);
            /* both forks of Phil[n] must be used */
            Q_ASSERT((me->fork[n] == USED) && (me->fork[m] == USED));

            me->fork[m] = FREE;
            me->fork[n] = FREE;
            m = RIGHT(n); /* check the right neighbor */

            if ((me->isHungry[m] != 0U) && (me->fork[m] == FREE)) {
                me->fork[n] = USED;
                me->fork[m] = USED;
                me->isHungry[m] = 0U;
                philo = (QActive *)Q_ROM_PTR(QF_active[PHILO_0_PRIO + m].act);
                QACTIVE_POST(philo, EAT_SIG, m);
                BSP_displayPhilStat(m, "eating  ");
            }
            m = LEFT(n); /* check the left neighbor */
            n = LEFT(m); /* left fork of the left neighbor */
            if ((me->isHungry[m] != 0U) && (me->fork[n] == FREE)) {
                me->fork[m] = USED;
                me->fork[n] = USED;
                me->isHungry[m] = 0U;
                philo = (QActive *)Q_ROM_PTR(QF_active[PHILO_0_PRIO + m].act);
                QACTIVE_POST(philo, EAT_SIG, m);
                BSP_displayPhilStat(m, "eating  ");
            }
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/1/2/1/2/2) */
        case EAT_SIG: {
            Q_ERROR();
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/1/2/1/2/3) */
        case PAUSE_SIG: {
            status_ = Q_TRAN(&Table_paused);
            break;
        }
        default: {
            status_ = Q_SUPER(&Table_active);
            break;
        }
    }
    return status_;
}
/* @(/1/1/2/1/3) ...........................................................*/
static QState Table_paused(Table * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* @(/1/1/2/1/3) */
        case Q_ENTRY_SIG: {
            BSP_displayPaused(1U);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/1/2/1/3) */
        case Q_EXIT_SIG: {
            BSP_displayPaused(0U);
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/1/2/1/3/0) */
        case PAUSE_SIG: {
            status_ = Q_TRAN(&Table_serving);
            break;
        }
        /* @(/1/1/2/1/3/1) */
        case HUNGRY_SIG: {
            uint8_t n = (uint8_t)(Q_PAR(me) - PHILO_0_PRIO);
            /* philo ID must be in range and he must be not hungry */
            Q_ASSERT((n < N_PHILO) && (me->isHungry[n] == 0U));
            me->isHungry[n] = 1U;
            BSP_displayPhilStat(n, "hungry  ");
            status_ = Q_HANDLED();
            break;
        }
        /* @(/1/1/2/1/3/2) */
        case DONE_SIG: {
            uint8_t n, m;

            n = (uint8_t)(Q_PAR(me) - PHILO_0_PRIO);
            /* phil ID must be in range and he must be not hungry */
            Q_ASSERT((n < N_PHILO) && (me->isHungry[n] == 0U));

            BSP_displayPhilStat(n, "thinking");
            m = LEFT(n);
            /* both forks of Phil[n] must be used */
            Q_ASSERT((me->fork[n] == USED) && (me->fork[m] == USED));

            me->fork[m] = FREE;
            me->fork[n] = FREE;
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Table_active);
            break;
        }
    }
    return status_;
}

