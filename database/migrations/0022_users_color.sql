-- Members are shown as a colored circle with their initials instead of a
-- generated DiceBear avatar. Each Member picks the color when their profile
-- is created; existing Members get one from the palette by id so they
-- don't all start the same. The palette lives in src/Models/MemberColors.cpp
-- and mobile/src/api/memberColors.ts.

ALTER TABLE users ADD COLUMN color TEXT;

UPDATE users SET color = (ARRAY[
    '#1f4a85', '#0f766e', '#2f855a', '#b7791f', '#c05621',
    '#c53030', '#b83280', '#6b46c1', '#4c51bf', '#4a5568'
])[(id % 10) + 1];

ALTER TABLE users ALTER COLUMN color SET DEFAULT '#1f4a85';
ALTER TABLE users ALTER COLUMN color SET NOT NULL;

ALTER TABLE users DROP COLUMN avatar_seed;
