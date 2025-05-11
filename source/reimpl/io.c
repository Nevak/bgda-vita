/*
 * reimpl/io.c
 *
 * Wrappers and implementations for some of the IO functions.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "reimpl/io.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <psp2/kernel/threadmgr.h>

//#ifdef USE_SCELIBC_IO
#include <libc_bridge/libc_bridge.h>
//#endif
#include "utils/logger.h"
#include "utils/utils.h"

#include <fios/fios.h>

// Includes the following inline utilities:
// int oflags_newlib_to_oflags_musl(int flags);
// dirent64_bionic * dirent_newlib_to_dirent_bionic(struct dirent* dirent_newlib);
// void stat_newlib_to_stat_bionic(struct stat * src, stat64_bionic * dst);
#include "_struct_converters.c"


char* existing_files[] =
{
    "/res/act1tran.lmp",
    "/res/act1trap.lmp",
    "/res/act2tran.lmp",
    "/res/act2trap.lmp",
    "/res/act3tran.lmp",
    "/res/act3trap.lmp",
    "/res/acttrans1.xmv",
    "/res/acttrans2.xmv",
    "/res/acttrans3.xmv",
    "/res/add_text.lmp",
    "/res/add_texture.lmp",
    "/res/alien1.lmp",
    "/res/alien2.lmp",
    "/res/armor.yak",
    "/res/barrel.lmp",
    "/res/bartend.lmp",
    "/res/bartley.lmp",
    "/res/baskets.lmp",
    "/res/baskets2.lmp",
    "/res/bat.lmp",
    "/res/behold.ico",
    "/res/beholder.lmp",
    "/res/bislogo.lmp",
    "/res/bislogo.xmv",
    "/res/bog1.gob",
    "/res/bog1.tex",
    "/res/bog1.va1",
    "/res/bog1.vat",
    "/res/bogtree.lmp",
    "/res/branoch.lmp",
    "/res/brogan.lmp",
    "/res/bthrone.gob",
    "/res/bthrone.tex",
    "/res/bthrone.va1",
    "/res/bthrone.vat",
    "/res/bthrone.vbe",
    "/res/bthrone.vbf",
    "/res/bthrone.vbg",
    "/res/bthrone.vbi",
    "/res/bthrone.vbs",
    "/res/bthrone2.vat",
    "/res/btube.lmp",
    "/res/bugbear.lmp",
    "/res/bulette.lmp",
    "/res/bullfrog.lmp",
    "/res/burneye1.gob",
    "/res/burneye1.tex",
    "/res/burneye1.va1",
    "/res/burneye1.vat",
    "/res/burneye2.gob",
    "/res/burneye2.tex",
    "/res/burneye2.va1",
    "/res/burneye2.vat",
    "/res/burneye3.gob",
    "/res/burneye3.tex",
    "/res/burneye3.va1",
    "/res/burneye3.vat",
    "/res/burneye4.gob",
    "/res/burneye4.tex",
    "/res/burneye4.va1",
    "/res/burneye4.vat",
    "/res/burnfrst.gob",
    "/res/burnfrst.tex",
    "/res/burnfrst.va1",
    "/res/burnfrst.vat",
    "/res/bush.lmp",
    "/res/cantent1.lmp",
    "/res/cantent2.lmp",
    "/res/canvtent.lmp",
    "/res/cat.lmp",
    "/res/cellar1.gob",
    "/res/cellar1.tex",
    "/res/cellar1.va1",
    "/res/cellar1.vat",
    "/res/cemetary.lmp",
    "/res/cguard.lmp",
    "/res/chapel.gob",
    "/res/chapel.tex",
    "/res/chapel.va1",
    "/res/chapel.vat",
    "/res/chekeep1.gob",
    "/res/chekeep1.tex",
    "/res/chekeep1.va1",
    "/res/chekeep1.vat",
    "/res/chekeep2.gob",
    "/res/chekeep2.tex",
    "/res/chekeep2.va1",
    "/res/chekeep2.vat",
    "/res/chekeep2.vbe",
    "/res/chekeep2.vbf",
    "/res/chekeep2.vbg",
    "/res/chekeep2.vbi",
    "/res/chekeep2.vbs",
    "/res/chelimb1.gob",
    "/res/chelimb1.tex",
    "/res/chelimb1.va1",
    "/res/chelimb1.vat",
    "/res/chelimb2.va1",
    "/res/chelobs.lmp",
    "/res/chest.lmp",
    "/res/chest2.lmp",
    "/res/chest3.lmp",
    "/res/config.lmp",
    "/res/cratea.lmp",
    "/res/crateb.lmp",
    "/res/crusher.lmp",
    "/res/crypt1.gob",
    "/res/crypt1.tex",
    "/res/crypt1.va1",
    "/res/crypt1.vat",
    "/res/crypt1.vbe",
    "/res/crypt1.vbf",
    "/res/crypt1.vbg",
    "/res/crypt1.vbi",
    "/res/crypt1.vbs",
    "/res/crypt2.gob",
    "/res/crypt2.tex",
    "/res/crypt2.va1",
    "/res/crypt2.vat",
    "/res/crypt2.vbe",
    "/res/crypt2.vbf",
    "/res/crypt2.vbg",
    "/res/crypt2.vbi",
    "/res/crypt2.vbs",
    "/res/crypt3.vat",
    "/res/cryptpal.lmp",
    "/res/crystal.lmp",
    "/res/cs_town.va1",
    "/res/csewer1.gob",
    "/res/csewer1.tex",
    "/res/csewer1.va1",
    "/res/csewer1.vat",
    "/res/csewer1.vbe",
    "/res/csewer1.vbf",
    "/res/csewer1.vbg",
    "/res/csewer1.vbi",
    "/res/csewer1.vbs",
    "/res/cube.lmp",
    "/res/cuttown.gob",
    "/res/cuttown.tex",
    "/res/cuttown.va1",
    "/res/cuttown.vat",
    "/res/cuttown.vbe",
    "/res/cuttown.vbf",
    "/res/cuttown.vbg",
    "/res/cuttown.vbi",
    "/res/cuttown.vbs",
    "/res/d_burn.lmp",
    "/res/d_cave.lmp",
    "/res/d_night.lmp",
    "/res/d_swamp.lmp",
    "/res/dbeast.lmp",
    "/res/dialogj.lmp",
    "/res/dragon.lmp",
    "/res/drizz.lmp",
    "/res/drizz_fr.lmp",
    "/res/drizz_ge.lmp",
    "/res/drizz_it.lmp",
    "/res/drizz_sp.lmp",
    "/res/drow_f.lmp",
    "/res/drow_m.lmp",
    "/res/drow_r.lmp",
    "/res/drunk.lmp",
    "/res/dwarf.lmp",
    "/res/dwarf_d.lmp",
    "/res/dwarf_f.lmp",
    "/res/dwarf_fr.lmp",
    "/res/dwarf_ge.lmp",
    "/res/dwarf_it.lmp",
    "/res/dwarf_m.lmp",
    "/res/dwarf_sp.lmp",
    "/res/eldrith.lmp",
    "/res/elf.lmp",
    "/res/elf_fr.lmp",
    "/res/elf_ge.lmp",
    "/res/elf_it.lmp",
    "/res/elf_sp.lmp",
    "/res/end.lmp",
    "/res/ending.xmv",
    "/res/endingf.xmv",
    "/res/endingg.xmv",
    "/res/endingi.xmv",
    "/res/endings.xmv",
    "/res/env_def.lmp",
    "/res/env_ice.lmp",
    "/res/env_shll.lmp",
    "/res/env_sink.lmp",
    "/res/env_thv.lmp",
    "/res/env_twr.lmp",
    "/res/equipmnt.lmp",
    "/res/ethon.lmp",
    "/res/fayed.lmp",
    "/res/fgiant.lmp",
    "/res/fireele.lmp",
    "/res/flag.lmp",
    "/res/flint.lmp",
    "/res/font.lmp",
    "/res/forcedr.lmp",
    "/res/forestbe.lmp",
    "/res/frontend.lmp",
    "/res/fx.lmp",
    "/res/g_brass.lmp",
    "/res/g_bronze.lmp",
    "/res/g_bsteel.lmp",
    "/res/g_onyx.lmp",
    "/res/g_steel.lmp",
    "/res/gargoyle.lmp",
    "/res/garik.lmp",
    "/res/gauntlet.gob",
    "/res/gauntlet.tex",
    "/res/gauntlet.va1",
    "/res/gauntlet.vat",
    "/res/gears.lmp",
    "/res/globsnd.lmp",
    "/res/gnoll.lmp",
    "/res/gtext.lmp",
    "/res/gtext_uk.lmp",
    "/res/gtman1.lmp",
    "/res/gtube.lmp",
    "/res/guns.lmp",
    "/res/hardwood.lmp",
    "/res/hoochie.lmp",
    "/res/horn.lmp",
    "/res/hud.lmp",
    "/res/human.lmp",
    "/res/human_fr.lmp",
    "/res/human_ge.lmp",
    "/res/human_it.lmp",
    "/res/human_sp.lmp",
    "/res/hutcloth.lmp",
    "/res/icebarel.lmp",
    "/res/icecave1.gob",
    "/res/icecave1.tex",
    "/res/icecave1.va1",
    "/res/icecave1.vat",
    "/res/icecave2.gob",
    "/res/icecave2.tex",
    "/res/icecave2.va1",
    "/res/icecave2.vat",
    "/res/icecave3.gob",
    "/res/icecave3.tex",
    "/res/icecave3.va1",
    "/res/icecave3.vat",
    "/res/icechst1.lmp",
    "/res/icecrate.lmp",
    "/res/icesheet.lmp",
    "/res/icon.lmp",
    "/res/icon.vat",
    "/res/icons.lmp",
    "/res/inventry.lmp",
    "/res/jfont.lmp",
    "/res/karne.lmp",
    "/res/keledon.lmp",
    "/res/kelp.lmp",
    "/res/kobold.lmp",
    "/res/kolgrim.lmp",
    "/res/kshaman.lmp",
    "/res/kveg.lmp",
    "/res/langmenu.lmp",
    "/res/legal.lmp",
    "/res/lever.lmp",
    "/res/lghtbeam.lmp",
    "/res/lilvarra.lmp",
    "/res/lizardm.lmp",
    "/res/load_fr.lmp",
    "/res/load_gr.lmp",
    "/res/load_it.lmp",
    "/res/load_sp.lmp",
    "/res/loading.lmp",
    "/res/mainmenu.lmp",
    "/res/mainmenu.va1",
    "/res/mapicons.lmp",
    "/res/medalion.lmp",
    "/res/mine1.gob",
    "/res/mine1.tex",
    "/res/mine1.va1",
    "/res/mine1.vat",
    "/res/mine2.gob",
    "/res/mine2.lmp",
    "/res/mine2.tex",
    "/res/mine2.va1",
    "/res/mine2.vat",
    "/res/mine3.gob",
    "/res/mine3.tex",
    "/res/mine3.va1",
    "/res/mine3.vat",
    "/res/mine3.vbe",
    "/res/mine3.vbf",
    "/res/mine3.vbg",
    "/res/mine3.vbi",
    "/res/mine3.vbs",
    "/res/minecamp.gob",
    "/res/minecamp.tex",
    "/res/minecamp.va1",
    "/res/minecamp.vat",
    "/res/minecamp.vbe",
    "/res/minecamp.vbf",
    "/res/minecamp.vbg",
    "/res/minecamp.vbi",
    "/res/minecamp.vbs",
    "/res/minedoor.lmp",
    "/res/minotaur.lmp",
    "/res/mouth.lmp",
    "/res/mouth.vat",
    "/res/mouth.vbe",
    "/res/mouth.vbf",
    "/res/mouth.vbg",
    "/res/mouth.vbi",
    "/res/mouth.vbs",
    "/res/mpgs.lmp",
    "/res/mtlcrate.lmp",
    "/res/nebbish.lmp",
    "/res/ogre.lmp",
    "/res/oilcan.lmp",
    "/res/onyxhr.lmp",
    "/res/onyxl1.lmp",
    "/res/onyxl2.lmp",
    "/res/orb.lmp",
    "/res/ox_chest.lmp",
    "/res/ox_plt.lmp",
    "/res/pbislogo.lmp",
    "/res/pend.lmp",
    "/res/plegal.lmp",
    "/res/pmainmnu.lmp",
    "/res/porko.lmp",
    "/res/porko.xmv",
    "/res/pot.lmp",
    "/res/powderk.lmp",
    "/res/pporko.lmp",
    "/res/pwotclog.lmp",
    "/res/ratgiant.lmp",
    "/res/ratsmall.lmp",
    "/res/rolldoor.lmp",
    "/res/rottrees.lmp",
    "/res/sackburl.lmp",
    "/res/sessth.lmp",
    "/res/sewer1.gob",
    "/res/sewer1.tex",
    "/res/sewer1.va1",
    "/res/sewer1.vat",
    "/res/sewer1.vbe",
    "/res/sewer1.vbf",
    "/res/sewer1.vbg",
    "/res/sewer1.vbi",
    "/res/sewer1.vbs",
    "/res/sewer1m.va1",
    "/res/sewer2.gob",
    "/res/sewer2.tex",
    "/res/sewer2.va1",
    "/res/sewer2.vat",
    "/res/sewer3.vat",
    "/res/shared",
    "/res/shared/barrel.lmp",
    "/res/shared/bugbear.lmp",
    "/res/shared/bush.lmp",
    "/res/shared/cemetary.lmp",
    "/res/shared/cube.lmp",
    "/res/shared/dbeast.lmp",
    "/res/shared/dome.lmp",
    "/res/shared/dragon.lmp",
    "/res/shared/drow_f.lmp",
    "/res/shared/drow_m.lmp",
    "/res/shared/drow_r.lmp",
    "/res/shared/fayed.lmp",
    "/res/shared/flag.lmp",
    "/res/shared/g_brass.lmp",
    "/res/shared/g_bronze.lmp",
    "/res/shared/g_bsteel.lmp",
    "/res/shared/g_onyx.lmp",
    "/res/shared/g_steel.lmp",
    "/res/shared/gargoyle.lmp",
    "/res/shared/gnoll.lmp",
    "/res/shared/gtman1.lmp",
    "/res/shared/hutcloth.lmp",
    "/res/shared/icebarel.lmp",
    "/res/shared/kobold.lmp",
    "/res/shared/kshaman.lmp",
    "/res/shared/lever.lmp",
    "/res/shared/minotaur.lmp",
    "/res/shared/ogre.lmp",
    "/res/shared/onyxl1.lmp",
    "/res/shared/ox_plt.lmp",
    "/res/shared/powderk.lmp",
    "/res/shared/ratgiant.lmp",
    "/res/shared/ratsmall.lmp",
    "/res/shared/reffiles.txt",
    "/res/shared/rolldoor.lmp",
    "/res/shared/shop.lmp",
    "/res/shared/skeleton.lmp",
    "/res/shared/sleyvas.lmp",
    "/res/shared/slime.lmp",
    "/res/shared/snowpkeg.lmp",
    "/res/shared/spore.lmp",
    "/res/shared/stonejar.lmp",
    "/res/shared/trees.lmp",
    "/res/shared/uhulk.lmp",
    "/res/shared/urns.lmp",
    "/res/shared/wolf.lmp",
    "/res/shared/yeti.lmp",
    "/res/shells1.gob",
    "/res/shells1.tex",
    "/res/shells2.tex",
    "/res/shells2.vat",
    "/res/shop.lmp",
    "/res/sigtower.gob",
    "/res/sigtower.tex",
    "/res/sigtower.va1",
    "/res/sigtower.vat",
    "/res/skeleton.lmp",
    "/res/sleyvas.lmp",
    "/res/slime.lmp",
    "/res/smlcave.va1",
    "/res/smlcave1.gob",
    "/res/smlcave1.tex",
    "/res/smlcave1.va1",
    "/res/smlcave1.vat",
    "/res/smlcave2.gob",
    "/res/smlcave2.tex",
    "/res/smlcave2.va1",
    "/res/smlcave2.vat",
    "/res/smlcave3.anm",
    "/res/smlcave3.gob",
    "/res/smlcave3.tex",
    "/res/smlcave3.va1",
    "/res/smlcave3.vat",
    "/res/smlcave3.vbe",
    "/res/smlcave3.vbf",
    "/res/smlcave3.vbg",
    "/res/smlcave3.vbi",
    "/res/smlcave3.vbs",
    "/res/smlcave4.gob",
    "/res/smlcave4.tex",
    "/res/smlcave4.va1",
    "/res/smlcave4.vat",
    "/res/snobarel.lmp",
    "/res/snowflag.lmp",
    "/res/snowpeak.gob",
    "/res/snowpeak.tex",
    "/res/snowpeak.va1",
    "/res/snowpeak.vat",
    "/res/snowpkeg.lmp",
    "/res/soundseq.lmp",
    "/res/spicebox.lmp",
    "/res/spider.lmp",
    "/res/spore.lmp",
    "/res/square.xmv",
    "/res/start.lmp",
    "/res/stonejar.lmp",
    "/res/swamp1.gob",
    "/res/swamp1.tex",
    "/res/swamp1.va1",
    "/res/swamp1.vat",
    "/res/swamp1.vbe",
    "/res/swamp1.vbf",
    "/res/swamp1.vbg",
    "/res/swamp1.vbi",
    "/res/swamp1.vbs",
    "/res/swamp2.gob",
    "/res/swamp2.tex",
    "/res/swamp2.va1",
    "/res/swamp2.vat",
    "/res/swamp2.vbe",
    "/res/swamp2.vbf",
    "/res/swamp2.vbg",
    "/res/swamp2.vbi",
    "/res/swamp2.vbs",
    "/res/tavern.gob",
    "/res/tavern.tex",
    "/res/tavern.va1",
    "/res/tavern.vat",
    "/res/tavern.vbe",
    "/res/tavern.vbf",
    "/res/tavern.vbg",
    "/res/tavern.vbi",
    "/res/tavern.vbs",
    "/res/temple1.gob",
    "/res/temple1.tex",
    "/res/temple1.va1",
    "/res/temple1.vat",
    "/res/temple1.vbe",
    "/res/temple1.vbf",
    "/res/temple1.vbg",
    "/res/temple1.vbi",
    "/res/temple1.vbs",
    "/res/thief.lmp",
    "/res/thief1.lmp",
    "/res/thief2.lmp",
    "/res/thief3.lmp",
    "/res/thieves1.gob",
    "/res/thieves1.tex",
    "/res/thieves1.va1",
    "/res/thieves1.vat",
    "/res/thieves2.gob",
    "/res/thieves2.lmp",
    "/res/thieves2.tex",
    "/res/thieves2.va1",
    "/res/thieves2.vat",
    "/res/thieves2.vbe",
    "/res/thieves2.vbf",
    "/res/thieves2.vbg",
    "/res/thieves2.vbi",
    "/res/thieves2.vbs",
    "/res/thieves3.gob",
    "/res/thieves3.tex",
    "/res/thieves3.va1",
    "/res/thieves3.vat",
    "/res/title.lmp",
    "/res/tkarne.lmp",
    "/res/tman1.lmp",
    "/res/tman2.lmp",
    "/res/torch.lmp",
    "/res/torrgeir.lmp",
    "/res/tower1.gob",
    "/res/tower1.tex",
    "/res/tower1.va1",
    "/res/tower1.vat",
    "/res/tower2.gob",
    "/res/tower2.tex",
    "/res/tower2.va1",
    "/res/tower2.vat",
    "/res/tower3.gob",
    "/res/tower3.tex",
    "/res/tower3.va1",
    "/res/tower3.vat",
    "/res/tower4.gob",
    "/res/tower4.tex",
    "/res/tower4.va1",
    "/res/tower4.vat",
    "/res/towerb.gob",
    "/res/towerb.tex",
    "/res/towerb.va1",
    "/res/towerb.vat",
    "/res/towerf.va1",
    "/res/towerh.gob",
    "/res/towerh.tex",
    "/res/towerh.va1",
    "/res/towerh.vat",
    "/res/towerh.vbe",
    "/res/towerh.vbf",
    "/res/towerh.vbg",
    "/res/towerh.vbi",
    "/res/towerh.vbs",
    "/res/towert.gob",
    "/res/towert.tex",
    "/res/towert.va1",
    "/res/towert.vat",
    "/res/towert.vbe",
    "/res/towert.vbf",
    "/res/towert.vbg",
    "/res/towert.vbi",
    "/res/towert.vbs",
    "/res/towertop.va1",
    "/res/town.gob",
    "/res/town.tex",
    "/res/town.va1",
    "/res/town.vat",
    "/res/town.vbe",
    "/res/town.vbf",
    "/res/town.vbg",
    "/res/town.vbi",
    "/res/town.vbs",
    "/res/treepine.lmp",
    "/res/trees.lmp",
    "/res/tutor.gob",
    "/res/tutor.tex",
    "/res/tutor.va1",
    "/res/tutor.vat",
    "/res/twoman1.lmp",
    "/res/twoman2.lmp",
    "/res/uhulk.lmp",
    "/res/uncle.lmp",
    "/res/urns.lmp",
    "/res/vines.lmp",
    "/res/w_cellar.lmp",
    "/res/w_chapel.lmp",
    "/res/w_chelim.lmp",
    "/res/w_forge.lmp",
    "/res/w_green.lmp",
    "/res/w_swamp.lmp",
    "/res/w_tower.lmp",
    "/res/waplants.lmp",
    "/res/watcher.lmp",
    "/res/watchmen.lmp",
    "/res/waterst.lmp",
    "/res/windwalk.gob",
    "/res/windwalk.tex",
    "/res/windwalk.va1",
    "/res/windwalk.vat",
    "/res/wine.lmp",
    "/res/wisp.lmp",
    "/res/wolf.lmp",
    "/res/wotc.xmv",
    "/res/wotclogo.lmp",
    "/res/yeti.lmp",
    "/res/zombie.lmp",
};



extern uint8_t psarc_exists;

FILE *fopen_soloader(char *fname, char *mode) {
    if (strcmp(fname, "/proc/cpuinfo") == 0) {
        return fopen_soloader("app0:/cpuinfo", mode);
    } else if (strcmp(fname, "/proc/meminfo") == 0) {
        return fopen_soloader("app0:/meminfo", mode);
    } else if (strcmp(fname, "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq") == 0) {
        return fopen_soloader("app0:/cpuinfo_max_freq", mode);
    } else if (strcmp(fname, "/sys/devices/system/cpu/present") == 0) {
        return fopen_soloader("app0:/present", mode);
    } else if (strcmp(fname, "/sys/devices/system/cpu/possible") == 0) {
        return fopen_soloader("app0:/possible", mode);
    }

    // this returns stuff like  0x81700010
    FILE* ret = sceLibcBridge_fopen(fname, mode);

    logv_debug("[io] fopen(%s, %s): 0x%x", fname, mode, ret);

    return ret;

}

int existing_files_len = sizeof(existing_files)/sizeof(existing_files[0]);

int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int retOpen = 0;
int open_soloader(char *_fname, int flags) {
    if (strcmp(_fname, "/proc/cpuinfo") == 0) {
        return open_soloader("app0:/cpuinfo", flags);
    } else if (strcmp(_fname, "/proc/meminfo") == 0) {
        return open_soloader("app0:/meminfo", flags);
    } else if (strcmp(_fname, "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq") == 0) {
        return open_soloader("app0:/cpuinfo_max_freq", flags);
    } else if (strcmp(_fname, "/sys/devices/system/cpu/present") == 0) {
        return open_soloader("app0:/present", flags);
    } else if (strcmp(_fname, "/sys/devices/system/cpu/possible") == 0) {
        return open_soloader("app0:/possible", flags);
    }

    SceFiosFH handle = 0;
    char real_fname[256];
    if (psarc_exists && !strncmp(_fname, "ux0:data/bgda/assets//res/", 26)) {
        // real name is whatever is after the prefix "ux0:data/bgda/assets//res/", so strip that
        // for example ux0:data/bgda/assets/res/add_texture.lmp should be /res/add_texture.lmp
        strcpy(real_fname, _fname + 21);


        // This weird optimization had to be done because Baba Is You on every
        // level/world calls fopen() for an unimaginable amount of non-existing
        // files. The following code reduced level load from ~18 min to ~25 s.
        char *real_fname_ptr = real_fname;
        char **existing_file = (char **)bsearch(&real_fname_ptr, existing_files, existing_files_len, sizeof(char *), compare_strings);
        if (existing_file == NULL) {
            logv_error("res file not found inside the existing_files list!!! %s\n", real_fname);
            return -1;
        }

        //logv_error("res file: %s", real_fname);
        // this returns stuff like 0x1800a
        int res = sceFiosFHOpenSync(NULL, &handle, real_fname, NULL);
        if (res != 0)
        {
            logv_error("res not found inside the PSARC!!! %s\n", real_fname);
            return -1;
        }
        
        // this returns stuff like 0x7fff8000
        int result = sceFiosFilenoToFH(handle);
        //int result = handle;
        logv_error("res file: %s, handle: 0x%x, result: 0x%x", real_fname, handle, result);

        // int size = sceFiosFHGetSize(handle);
        // logv_error("size: %i", size);

        // int fseekRes = sceFiosFHSeek(handle, 0, 2);
        // logv_error("fseekRes: %i", fseekRes);
        return result;

        // if (res < 0) {
        //     logv_error("res not found inside the PSARC!!! %s\n", real_fname);
        // } else {
        //     if (f == NULL) {
        //         logv_error("res not found inside the PSARC!!! %s\n", real_fname);
        //     } else {
        //         logv_debug("res found inside the PSARC!!! %s\n", real_fname);
        //         return res;
        //     }
        // }
    }

    flags = oflags_newlib_to_oflags_musl(flags);
    int ret = open(_fname, flags);
    // if (!strncmp(_fname, "ux0:data/bgda/assets//res/", 26))
    // {
    //     logv_debug("[io] open(%s, %x): %i", _fname, flags, ret);
    //     retOpen = ret;
    // }
    //logv_debug("[io] open(%s, %x): %i", _fname, flags, ret);
    return ret;
}

int fstat_soloader(int fd, void *statbuf) {
    struct stat st;
    int res = fstat(fd, &st);
    if (res == 0)
        stat_newlib_to_stat_bionic(&st, statbuf);

    logv_debug("[io] fstat(fd#%i): %i", fd, res);
    return res;
}

int stat_soloader(char *_pathname, stat64_bionic *statbuf) {
    struct stat st;
    int res = stat(_pathname, &st);

    if (res == 0)
        stat_newlib_to_stat_bionic(&st, statbuf);

    //logv_debug("[io] stat(%s): %i", _pathname, res);
    return res;
}

int fclose_soloader(FILE * f) {
    int ret = sceLibcBridge_fclose(f);

    logv_debug("[io] fclose(0x%x): %i", f, ret);
    return ret;
}

int close_soloader(int fd) {
    uint32_t fiosH = sceFiosFHToFileno(fd);
	if (fiosH == 0xffffffff)
	{
        int ret = close(fd);
       // logv_debug("[io]non-fios close(fd#%i): %i", fd, ret);
        return ret;
    }
    else
    {
        logv_debug("[io] close(fd#0x%x), fiosH=0x%x", fd, fiosH);
        int ret = sceFiosFHCloseSync(NULL, fiosH);
        logv_debug("[io] return close(fd#0x%x): %i", fd, ret);
        return ret;
    }
}

DIR* opendir_soloader(char* _pathname) {
    DIR* ret = opendir(_pathname);
    logv_debug("[io] opendir(\"%s\"): 0x%x", _pathname, ret);
    return ret;
}

struct dirent64_bionic * readdir_soloader(DIR * dir) {
    static struct dirent64_bionic dirent_tmp;

    struct dirent* ret = readdir(dir);
    logv_debug("[io] readdir(%p): %p", dir, ret);

    if (ret) {
        dirent64_bionic* entry_tmp = dirent_newlib_to_dirent_bionic(ret);
        memcpy(&dirent_tmp, entry_tmp, sizeof(dirent64_bionic));
        free(entry_tmp);
        //logv_debug("  [io] readdir(%p): %s", dir, dirent_tmp.d_name);
        return &dirent_tmp;
    }

    return NULL;
}

int readdir_r_soloader(DIR *dirp, dirent64_bionic *entry, dirent64_bionic **result) {
    struct dirent dirent_tmp;
    struct dirent* pdirent_tmp;

    int ret = readdir_r(dirp, &dirent_tmp, &pdirent_tmp);

    if (ret == 0) {
        dirent64_bionic* entry_tmp = dirent_newlib_to_dirent_bionic(&dirent_tmp);
        memcpy(entry, entry_tmp, sizeof(dirent64_bionic));
        *result = (pdirent_tmp != NULL) ? entry : NULL;
        free(entry_tmp);
    }

    log_debug("[io] readdir_r()");
    return ret;
}

int closedir_soloader(DIR* dir) {
    int ret = closedir(dir);
    logv_debug("[io] closedir(0x%x): %i", dir, ret);
    return ret;
}

int fcntl_soloader(int fd, int cmd, ...) {
    logv_debug("[io] fcntl(fd#%i, cmd#%i)", fd, cmd);
    return 0;
}

int fsync_soloader(int fd) {
    int ret = fsync(fd);
    logv_debug("[io] fsync(%i): %i", fd, ret);
    return ret;
}

size_t fread_soloader(void *p, size_t size, size_t num, FILE *f) {
    logv_debug("[io] fread(%p, %i, %i, 0x%x)", p, size, num, f);
	return sceLibcBridge_fread(p, size, num, f);
}

int fstat_hook(int fd, void *statbuf) {
	struct stat st;
	int res = fstat(fd, &st);
	if (res == 0)
		*(uint64_t *)(statbuf + 0x30) = st.st_size;
	return res;
}

int fseek_soloader(FILE *f, int dist, int off) {
    logv_debug("[io] fseek(0x%x, %i, %i)", f, dist, off);
	return sceLibcBridge_fseek(f, dist, off);
}

long ftell_soloader(FILE *f) {
    logv_debug("[io] ftell(0x%x)", f);
	return sceLibcBridge_ftell(f);
}