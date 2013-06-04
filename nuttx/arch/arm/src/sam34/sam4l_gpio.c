/****************************************************************************
 * arch/arm/src/sam34/sam4l_gpio.c
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <arch/board/board.h>

#include "up_internal.h"
#include "up_arch.h"

#include "chip.h"
#include "sam_gpio.h"
#include "chip/sam4l_gpio.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_DEBUG_GPIO
static const char g_portchar[4]   = { 'A', 'B', 'C', 'D' };
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
/****************************************************************************
 * Name: sam_gpiobase
 *
 * Description:
 *   Return the base address of the GPIO register set
 *
 ****************************************************************************/

static inline uintptr_t sam_gpiobase(gpio_pinset_t cfgset)
{
  int port = (cfgset & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
  return SAM_GPION_BASE(port);
}

/****************************************************************************
 * Name: sam_gpiopin
 *
 * Description:
 *   Returun the base address of the GPIO register set
 *
 ****************************************************************************/

static inline int sam_gpiopin(gpio_pinset_t cfgset)
{
  return 1 << ((cfgset & GPIO_PIN_MASK) >> GPIO_PIN_SHIFT);
}

/****************************************************************************
 * Name: sam_configinput
 *
 * Description:
 *   Configure a GPIO input pin based on bit-encoded description of the pin.
 *
 ****************************************************************************/

static int sam_configinput(uintptr_t base, uint32_t pin, gpio_pinset_t cfgset)
{
  /* Disable interrupts on the pin */

  putreg32(pin, base + SAM_GPIO_IERC_OFFSET);

  /* Disable the output driver and select the GPIO function */

  putreg32(pin, base + SAM_GPIO_ODERC_OFFSET);
  putreg32(pin, base + SAM_GPIO_GPERS_OFFSET);

  /* Clear peripheral-only settings (just to make debug easier) */

  putreg32(pin, base + SAM_GPIO_PMR0C_OFFSET);
  putreg32(pin, base + SAM_GPIO_PMR1C_OFFSET);
  putreg32(pin, base + SAM_GPIO_PMR2C_OFFSET);
  putreg32(pin, base + SAM_GPIO_EVERC_OFFSET);

  /* Clear output-only settings (just to make debug easier) */

  putreg32(pin, base + SAM_GPIO_ODCR0C_OFFSET);
  putreg32(pin, base + SAM_GPIO_ODCR1C_OFFSET);
  putreg32(pin, base + SAM_GPIO_OSRR0C_OFFSET);

  /* Clear the interrupt configuration (just to make debug easier) */

  putreg32(pin, base + SAM_GPIO_IMR0C_OFFSET);
  putreg32(pin, base + SAM_GPIO_IMR1C_OFFSET);

  /* Enable/disable the pull-up as requested */

  if ((cfgset & GPIO_PULL_UP) != 0)
    {
      putreg32(pin, base + SAM_GPIO_PUERS_OFFSET);
    }
  else
    {
      putreg32(pin, base + SAM_GPIO_PUERC_OFFSET);
    }

  if ((cfgset & GPIO_PULL_DOWN) != 0)
    {
      putreg32(pin, base + SAM_GPIO_PDERS_OFFSET);
    }
  else
    {
      putreg32(pin, base + SAM_GPIO_PDERC_OFFSET);
    }

  /* Check if glitch filtering should be enabled */

  if ((cfgset & GPIO_GLITCH_FILTER) != 0)
    {
      putreg32(pin, base + SAM_GPIO_GFERS_OFFSET);
    }
  else
    {
      putreg32(pin, base + SAM_GPIO_GFERC_OFFSET);
    }

  /* Check if the input Schmitt trigger should be enabled */

  if ((cfgset & GPIO_SCHMITT_TRIGGER) != 0)
    {
      putreg32(pin, base + SAM_GPIO_STERS_OFFSET);
    }
  else
    {
      putreg32(pin, base + SAM_GPIO_STERC_OFFSET);
    }

 return OK;
}

/****************************************************************************
 * Name: sam_configinterrupt
 *
 * Description:
 *   Configure a GPIO input pin based on bit-encoded description of the pin.
 *
 ****************************************************************************/

static inline int sam_configinterrupt(uintptr_t base, uint32_t pin,
                                      gpio_pinset_t cfgset)
{
  int ret;

  /* Just configure the pin as an input, then set the interrupt configuration.
   * Here we exploit the fact that sam_configinput() enabled both rising and
   * falling edges.
   */

  ret = sam_configinput(base, pin, cfgset)
  if (ret = OK)
    {
      /* Disable rising and falling edge interrupts as requested 
       * {IMR1, IMR0} Interrupt Mode
       *
       *   00 Pin Change   <-- We already have this
       *   01 Rising Edge  <-- GPIO_INT_RISING
       *   10 Falling Edge <-- GPIO_INT_FALLING
       *   11 Reserved
       */

      gpio_pinset_t edges = cfgset & GPIO_INT_MASK;

      if (eges = GPIO_INT_RISING)
        {
          /* Rising only.. disable interrrupts on the falling edge */

          putreg32(pin, base + SAM_GPIO_IMR0S_OFFSET);
        }
      else if (edges = GPIO_INT_FALLING)
        {
          /* Falling only.. disable interrrupts on the rising edge */

          putreg32(pin, base + SAM_GPIO_IMR1S_OFFSET);
        }
    }

  return ret;
}

/****************************************************************************
 * Name: sam_configoutput
 *
 * Description:
 *   Configure a GPIO output pin based on bit-encoded description of the pin.
 *
 * Assumption:
 *   sam_configinput has been called to put the pin into the default input
 *   state:
 *
 *     GPER              -> GPIO
 *     PMD0-2            -> zeroed
 *     ODER              -> disabled
 *     PUER+PDER         -> No pull up- or down.
 *     IER               -> Interrupt disabled
 *     IMR0-1            -> zeroed
 *     Glitch filter     -> disabled
 *     Output drive      -> lowest
 *     Slew control      -> disabled
 *     Schmitt trigger   -> disabled
 *     Peripheral events -> disabled
 *
 ****************************************************************************/

static inline int sam_configoutput(uintptr_t base, uint32_t pin,
                                   gpio_pinset_t cfgset)
{
  /* Set the output drive strength
   *
   * {ODCR1, ODCR0} Output drive strength
   *
   *   00 Lowest drive strength
   *   01 ...
   *   10 ...
   *   11 Highest drive strength
   */

  switch (cfgset & GPIO_DRIVE_MASK)
    {
      default:
      case GPIO_DRIVE_LOW:     /* OCDR1=0 OCDR0=0 */
        break;                 /* This is the current setting */

      case GPIO_DRIVE_MEDLOW:  /* OCDR1=0 OCDR0=1 */
        putreg32(pin, base + SAM_GPIO_ODCR0S_OFFSET);
        break;

      case GPIO_DRIVE_MEDHIGH: /* OCDR1=1 OCDR0=0 */
        putreg32(pin, base + SAM_GPIO_ODCR1S_OFFSET);
        break;

      case GPIO_DRIVE_HIGH:    /* OCDR1=1 OCDR0=1 */
        putreg32(pin, base + SAM_GPIO_ODCR0S_OFFSET);
        putreg32(pin, base + SAM_GPIO_ODCR1S_OFFSET);
        break;
    }

  /* Set the output slew control is requested */

  if ((cfgset & GPIO_SLEW) != 0)
    {
      putreg32(pin, base + SAM_GPIO_OSRR0S_OFFSET);
    }

  /* Enable the output driver */

  putreg32(pin, base + SAM_GPIO_ODERS_OFFSET);

  /* And set the initial value of the output */

  sam_gpiowrite(cfgset, ((cfgset & GPIO_OUTPUT_SET) != 0));
  return OK;
}

/****************************************************************************
 * Name: sam_configperiph
 *
 * Description:
 *   Configure a GPIO pin driven by a peripheral based on bit-encoded
 *   description of the pin.
 *
 * Assumption:
 *   sam_configinput has been called to put the pin into the default input
 *   state:
 *
 *     GPER              -> GPIO
 *     PMD0-2            -> zeroed
 *     ODER              -> disabled
 *     PUER+PDER         -> No pull up- or down.
 *     IER               -> Interrupt disabled
 *     IMR0-1            -> zeroed
 *     Glitch filter     -> disabled
 *     Output drive      -> lowest
 *     Slew control      -> disabled
 *     Schmitt trigger   -> disabled
 *     Peripheral events -> disabled
 *
 ****************************************************************************/

static inline int sam_configperiph(uintptr_t base, uint32_t pin,
                                   gpio_pinset_t cfgset)
{
 *   Peripheral:  MM.. FFFE .... IIG. .PPB BBBB

  gpio_pinset_t edges;

  /* Select the peripheral function.
   *
   * {PMR2, PMR1, PMR0} selects peripheral function:
   *
   *   000 A    100 E
   *   001 B    101 F
   *   010 C    110 G
   *   011 D    111 H
   */

  switch (cfgset & GPIO_FUNC_MASK)
    {
      default:
      case _GPIO_FUNCA: /* Function A 000 */
        break;          /* We already have this configuration */

      case _GPIO_FUNCD: /* Function D 011 */
        putreg32(pin, base + SAM_GPIO_PMR1S_OFFSET);
        break;
      case _GPIO_FUNCB: /* Function B 001 */
        putreg32(pin, base + SAM_GPIO_PMR0S_OFFSET);
        break;

      case _GPIO_FUNCG: /* Function G 110 */
        putreg32(pin, base + SAM_GPIO_PMR2S_OFFSET);
      case _GPIO_FUNCC: /* Function C 010 */
        putreg32(pin, base + SAM_GPIO_PMR1S_OFFSET);
        break;
 
      case _GPIO_FUNCE: /* Function E 100 */
        putreg32(pin, base + SAM_GPIO_PMR2S_OFFSET);
        break;
      case _GPIO_FUNCF: /* Function F 101 */
        putreg32(pin, base + SAM_GPIO_PMR0S_OFFSET);
        break;

      case _GPIO_FUNCH: /* Function H 111 */
        putreg32(pin, base + SAM_GPIO_PMR0S_OFFSET);
        putreg32(pin, base + SAM_GPIO_PMR1S_OFFSET);
        putreg32(pin, base + SAM_GPIO_PMR2S_OFFSET);
        break;
    }

  /* Check if glitch filtering should be enabled */

  if ((cfgset & GPIO_GLITCH_FILTER) != 0)
    {
      putreg32(pin, base + SAM_GPIO_GFERS_OFFSET);
    }

  /* Disable rising and falling edge events as requested (of course,
   * these do nothing unless events are also enabled.
   *
   * {IMR1, IMR0} Interrupt Mode
   *
   *   00 Pin Change   <-- We already have this
   *   01 Rising Edge  <-- GPIO_INT_RISING
   *   10 Falling Edge <-- GPIO_INT_FALLING
   *   11 Reserved
   */

  edges = cfgset & GPIO_INT_MASK;
  if (eges = GPIO_INT_RISING)
    {
      /* Rising only.. disable interrrupts on the falling edge */

      putreg32(pin, base + SAM_GPIO_IMR0S_OFFSET);
    }
  else if (edges = GPIO_INT_FALLING)
    {
      /* Falling only.. disable interrrupts on the rising edge */

      putreg32(pin, base + SAM_GPIO_IMR1S_OFFSET);
    }

 /* REVISIT:  Should event generation be enabled now?  I am assuming so */

  if (eges = GPIO_PERIPH_EVENTS)
    {
      /* Rising only.. disable interrrupts on the falling edge */

      putreg32(pin, base + SAM_GPIO_EVERS_OFFSET);
    }

 /* Finally, drive the pen from the peripheral */

  putreg32(pin, base + SAM_GPIO_GPERC_OFFSET);
  return OK;
}

/****************************************************************************
 * Global Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sam_configgpio
 *
 * Description:
 *   Configure a GPIO pin based on bit-encoded description of the pin.
 *
 ****************************************************************************/

int sam_configgpio(gpio_pinset_t cfgset)
{
  gpio_pinset_t inputset;
  uintptr_t base = sam_gpiobase(cfgset);
  uint32_t pin = sam_gpiopin(cfgset);
  int ret;

  /* Put the GPIO in a known state.  A generic GPIO input pin. */

  inputset = GPIO_INPUT | (cfgset & (GPIO_PORT_MASK | GPIO_PIN_MASK));
  ret = sam_configinput(base, pin, inputset);
  if (ret == OK)
    {
      /* Then put the GPIO into the requested state */

      switch (cfgset & GPIO_MODE_MASK)
        {
          case GPIO_INPUT:
            ret = sam_configinput(base, pin, cfgset);
            break;

          case GPIO_OUTPUT:
            ret = sam_configoutput(base, pin, cfgset);
            break;

          case GPIO_PERIPHERAL:
            ret = sam_configperiph(base, pin, cfgset);
            break;

          case GPIO_INTERRUPT:
            ret = sam_configinterrupt(base, pin, cfgset);
            break;

          default:
            ret = -EINVAL;
            break;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: sam_gpiowrite
 *
 * Description:
 *   Write one or zero to the selected GPIO pin
 *
 ****************************************************************************/

void sam_gpiowrite(gpio_pinset_t pinset, bool value)
{
  uintptr_t base = sam_gpiobase(pinset);
  uint32_t  pin  = sam_gpiopin(pinset);

  if (value)
    {
      putreg32(pin, base + SAM_GPIO_OVRS_OFFSET);
    }
  else
    {
      putreg32(pin, base + SAM_GPIO_OVRC_OFFSET);
    }
}

/****************************************************************************
 * Name: sam_gpioread
 *
 * Description:
 *   Read one or zero from the selected GPIO pin
 *
 ****************************************************************************/

bool sam_gpioread(gpio_pinset_t pinset)
{
  uintptr_t base = sam_gpiobase(pinset);
  uint32_t  pin  = sam_gpiopin(pinset);

  return (getreg32(base + SAM_GPIO_PVR_OFFSET) & pin) != 0;
}

/************************************************************************************
 * Function:  sam_dumpgpio
 *
 * Description:
 *   Dump all GPIO registers associated with the base address of the provided pinset.
 *
 ************************************************************************************/

#ifdef CONFIG_DEBUG_GPIO
int sam_dumpgpio(uint32_t pinset, const char *msg)
{
  irqstate_t    flags;
  uintptr_t     base;
  unsigned int  pin;
  unsigned int  port;

  /* Get the base address associated with the PIO port */

  pin  = sam_gpiopin(pinset);
  port = (pinset & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
  base = SAM_GPION_BASE(port);

  /* The following requires exclusive access to the GPIO registers */

  flags = irqsave();
  lldbg("GPIO%c pinset: %08x base: %08x -- %s\n",
        g_portchar[port], pinset, base, msg);
  lldbg("    GPER: %08x  PMR0: %08x  PMR1: %08x  PMR2: %08x\n",
        getreg32(base + SAM_GPIO_GPER_OFFSET), getreg32(base + SAM_GPIO_PMR0_OFFSET),
        getreg32(base + SAM_GPIO_PMR1_OFFSET), getreg32(base + SAM_GPIO_PMR2_OFFSET));
  lldbg("   ODER: %08x   OVR: %08x   PVR:  %08x  PUER: %08x\n",
        getreg32(base + SAM_GPIO_ODER_OFFSET), getreg32(base + SAM_GPIO_OVR_OFFSET),
        getreg32(base + SAM_GPIO_PVR_OFFSET), getreg32(base + SAM_GPIO_PUER_OFFSET));
  lldbg("   PDER: %08x    IER: %08x  IMR0: %08x  IMR1: %08x\n",
        getreg32(base + SAM_GPIO_PDER_OFFSET), getreg32(base + SAM_GPIO_IER_OFFSET),
        getreg32(base + SAM_GPIO_IMR0_OFFSET), getreg32(base + SAM_GPIO_IMR1_OFFSET));
  lldbg("   GFER: %08x    IFR: %08x ODCR0: %08x ODCR1: %08x\n",
        getreg32(base + SAM_GPIO_GFER_OFFSET), getreg32(base + SAM_GPIO_IFR_OFFSET),
        getreg32(base + SAM_GPIO_ODCR0_OFFSET), getreg32(base + SAM_GPIO_ODCR1_OFFSET));
  lldbg("  OSRR0: %08x   EVER: %08x PARAM: %08x  VERS: %08x\n",
        getreg32(base + SAM_GPIO_OSRR0_OFFSET), getreg32(base + SAM_GPIO_EVER_OFFSET),
        getreg32(base + SAM_GPIO_PARAMETER_OFFSET), getreg32(base + SAM_GPIO_VERSION_OFFSET));
  irqrestore(flags);
  return OK;
}
#endif
