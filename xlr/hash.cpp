// ****************************************************************************
//  hash.cpp                                                       XLR project
// ****************************************************************************
//
//   File Description:
//
//    Simple SHA-based hashing algorithm for trees
//
//
//
//
//
//
//
//
// ****************************************************************************
// This document is released under the GNU General Public License, with the
// following clarification and exception.
//
// Linking this library statically or dynamically with other modules is making
// a combined work based on this library. Thus, the terms and conditions of the
// GNU General Public License cover the whole combination.
//
// As a special exception, the copyright holders of this library give you
// permission to link this library with independent modules to produce an
// executable, regardless of the license terms of these independent modules,
// and to copy and distribute the resulting executable under terms of your
// choice, provided that you also meet, for each linked independent module,
// the terms and conditions of the license of that module. An independent
// module is a module which is not derived from or based on this library.
// If you modify this library, you may extend this exception to your version
// of the library, but you are not obliged to do so. If you do not wish to
// do so, delete this exception statement from your version.
//
// See http://www.gnu.org/copyleft/gpl.html and Matthew 25:22 for details
//  (C) 1992-2010 Christophe de Dinechin <christophe@taodyne.com>
//  (C) 2010 Taodyne SAS
// ****************************************************************************

#include "hash.h"
#include "sha1_ostream.h"
#include <sstream>

XL_BEGIN

text sha1(Tree *t)
// ----------------------------------------------------------------------------
//    Compute the SHA-1 for a tree and return it
// ----------------------------------------------------------------------------
{
    text result;
    if (t)
    {
        TreeHashAction<> hash;
        t->Do(hash);

        std::ostringstream os;
        os << t->Get< HashInfo<> > ();

        result = os.str();
    }
    return result;
}

XL_END
