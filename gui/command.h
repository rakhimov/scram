/*
 * Copyright (C) 2017-2018 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/// @file
/// Undo-redo helper facilities based on the Qt Undo framework.
///
/// Undo-redo facilities assume the following contract:
///
/// 0. The facilities are special purpose to work with the undo-redo stack.
/// 1. The redo is always called first.
/// 2. The undo-redo functions are not required to be idempotent.
///    Calling the redo (or undo) consecutively yields undefined behavior.
///    In other words, the redo can only be followed by the undo,
///    and the undo can only be followed by the redo.
/// 3. The system state is only changed with the undo-redo facilities.
/// 4. As a consequence of contract #2 and #3,
///    the implementation can and should optimize the state storage
///    to hold only single snapshot data (no duplication of the state).
///    That is, the redo stage saves the past (by overwriting the future),
///    and the undo stage saves the future by overwriting the saved past.
/// 5. Objects (undo-redo arguments) must be alive and have stable addresses for
///    at least as long as there's a referencing undo-redo command in the stack.
///    Constructive/destructive commands extend
///    the life-time of an object for this reason.
///    That is, destructive commands do not destroy/deallocate at redo
///    or re-construct/allocate at undo (vice-versa for the constructive).
///    The object is destroyed/deallocated
///    after its corresponding constructive/destructive commands are destroyed
///    (e.g., by being popped/removed from the undo stack).

#pragma once

#include <type_traits>

#include <QUndoCommand>

namespace scram::gui {

/// The function inverse is the function itself (i.e., f(f(x)) = id(x)).
/// In other words, undo and redo codes are exactly the same,
/// but the arguments are different (one's output is the other's argument).
/// In this case, the argument is the state of the object or system.
class Involution : public QUndoCommand
{
public:
    using QUndoCommand::QUndoCommand;

    /// The redo is always called first;
    /// therefore, undo is implemented in terms of redo.
    void undo() final { return redo(); }
};

/// A command that is an inverse of another command.
///
/// @tparam T  The undo-redo command type.
///
/// @pre T command can tolerate the undo before the redo.
template <class T>
class Inverse : public T
{
    static_assert(std::is_base_of_v<QUndoCommand, T>);

public:
    /// Applies the command.
    void redo() final { T::undo(); }

    /// Reverses the command.
    void undo() final { T::redo(); }

protected:
    using T::T;
};

} // namespace scram::gui
