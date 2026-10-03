-- Members sign in with their phone number instead of an email address.
-- Every existing profile is removed rather than left without a way to
-- sign in; the first profile created afterwards becomes the Admin (see
-- /api/auth/register). Their sessions, away Sundays and absence requests
-- go with them (ON DELETE CASCADE); duties and feedback stay, with no
-- member.
--
-- Phone numbers are stored normalized -- digits only, with a leading "+"
-- kept when given (UserController::normalizePhone) -- so "07700 900123"
-- and "07700900123" are the same account.
--
-- The color is no longer limited to a palette: any "#rrggbb" is allowed
-- (Models/MemberColors::isValid).
DELETE FROM users;

DROP INDEX users_email_key;
ALTER TABLE users DROP COLUMN email;

ALTER TABLE users ADD COLUMN phone TEXT NOT NULL;
CREATE UNIQUE INDEX users_phone_key ON users(phone);
